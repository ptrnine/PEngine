#include "grx_cascaded_shadow_mapping_tech.hpp"
#include <core/types.hpp>
#include <core/container_extensions.hpp>
#include <core/assert.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <GL/glew.h>

#include "algorithms/grx_frustum_culling.hpp"
#include "grx_mesh_instance.hpp"
#include "grx_camera.hpp"
#include "grx_shader_tech.hpp"

using namespace core;

namespace grx
{
grx_cascade_shadow_map_tech::grx_cascade_shadow_map_tech(
        const vec2u& size,
        size_t maps_count,
        const vector<float>& z_bounds
):
    _size(size),
    _shadow_maps(maps_count),
    _ortho_projections(maps_count),
    _light_projections(maps_count),
    _z_bounds(z_bounds),
    _gl_texture_nums(maps_count),
    _csm_end_cs(maps_count)
{
    RASSERT(z_bounds.size() - 1 == maps_count);

    glGenFramebuffers(1, &_fbo);

    glGenTextures(static_cast<GLsizei>(_shadow_maps.size()), _shadow_maps.data());

    for (auto& shadow_map : _shadow_maps) {
        glBindTexture(GL_TEXTURE_2D, shadow_map);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_DEPTH_COMPONENT32,
                static_cast<GLsizei>(_size.x()),
                static_cast<GLsizei>(_size.y()),
                0,
                GL_DEPTH_COMPONENT,
                GL_FLOAT,
                nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    for (auto shadow_map : _shadow_maps)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_map, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    auto rc = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    RASSERTF(rc == GL_FRAMEBUFFER_COMPLETE, "Shadow map framebuffer error: {}", rc);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

grx_cascade_shadow_map_tech::~grx_cascade_shadow_map_tech() {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(static_cast<GLsizei>(_shadow_maps.size()), _shadow_maps.data());
}

void grx_cascade_shadow_map_tech::set_z_bounds(const vector<float>& values) {
    RASSERT(static_cast<size_t>(values.size()) == _z_bounds.size());
    _z_bounds = values;
}

void grx_cascade_shadow_map_tech::cover_view(const shared_ptr<grx_camera>& camera, const vec3f& dir) {
    auto inv_view   = glm::inverse(camera->view());
    _light_view     = glm::translate(glm::lookAt({0.f, 0.f, 0.f}, to_glm(dir), {0.f, 1.f, 0.f}), to_glm(-camera->position()));

    auto aspect_ratio   = static_cast<float>(_size.x()) / static_cast<float>(_size.y());
    auto tan_half_h_fov = glm::tan(glm::radians(camera->vertical_fov() * aspect_ratio / 2.f));
    auto tan_half_v_fov = glm::tan(glm::radians(camera->vertical_fov() / 2.f));

    for (auto i : index_view(_shadow_maps)) {
        float xn = _z_bounds[i]     * tan_half_h_fov;
        float xf = _z_bounds[i + 1] * tan_half_h_fov;
        float yn = _z_bounds[i]     * tan_half_v_fov;
        float yf = _z_bounds[i + 1] * tan_half_v_fov;

        auto frustum = array{
            glm::vec4{ xn,  yn, -_z_bounds[i],     1.f},
            glm::vec4{-xn,  yn, -_z_bounds[i],     1.f},
            glm::vec4{ xn, -yn, -_z_bounds[i],     1.f},
            glm::vec4{-xn, -yn, -_z_bounds[i],     1.f},
            glm::vec4{ xf,  yf, -_z_bounds[i + 1], 1.f},
            glm::vec4{-xf,  yf, -_z_bounds[i + 1], 1.f},
            glm::vec4{ xf, -yf, -_z_bounds[i + 1], 1.f},
            glm::vec4{-xf, -yf, -_z_bounds[i + 1], 1.f}
        };

        vec min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        vec max = {std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()};

        for (auto& f : frustum) {
            auto lss = _light_view * (inv_view * f);

            min.x() = std::min(min.x(), lss.x); // NOLINT
            max.x() = std::max(max.x(), lss.x); // NOLINT
            min.y() = std::min(min.y(), lss.y); // NOLINT
            max.y() = std::max(max.y(), lss.y); // NOLINT
            min.z() = std::min(min.z(), lss.z); // NOLINT
            max.z() = std::max(max.z(), lss.z); // NOLINT
        }

        _ortho_projections[i].left   = min.x();
        _ortho_projections[i].right  = max.x();
        _ortho_projections[i].bottom = min.y();
        _ortho_projections[i].top    = max.y();
        _ortho_projections[i].near   = min.z();
        _ortho_projections[i].far    = max.z();

        _light_projections[i] = glm::ortho(
                _ortho_projections[i].left,
                _ortho_projections[i].right,
                _ortho_projections[i].bottom,
                _ortho_projections[i].top,
                _ortho_projections[i].near > -1500.f ? -1500.f : _ortho_projections[i].near,
                _ortho_projections[i].far  <  1500.f ?  1500.f : _ortho_projections[i].far
        );
        //core::printline("{.5f} {.5f} {.5f} {.5f} {.5f} {.5f}",
        //        _ortho_projections[i].left, _ortho_projections[i].right,
        //        _ortho_projections[i].bottom, _ortho_projections[i].top,
        //        _ortho_projections[i].near, _ortho_projections[i].far);

        auto v_view = glm::vec4{0.f, 0.f, _z_bounds[i + 1], 1.f};
        auto v_clip = camera->projection() * v_view;
        _csm_end_cs[i] = v_clip.z; // NOLINT
    }
}

void grx_cascade_shadow_map_tech::bind_framebuffer() {
    //glDisable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glViewport(0, 0, static_cast<GLsizei>(_size.x()), static_cast<GLsizei>(_size.y()));
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

    for (auto shadow_map : _shadow_maps) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_map, 0);
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void grx_cascade_shadow_map_tech::draw(const shared_ptr<grx_shader_program>& shader, grx_mesh_instance& mesh_instance) {
    // We has shadom_maps.size() * mesh_instance_count glFramebufferTexture calls now
    // Worst, TODO: make only 3 glFramebufferTexture calls for all meshes
    // Maybe frustum culling will help with that
    for (auto& [shadow_map, light_projection] : zip_view(_shadow_maps, _light_projections)) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_map, 0);
        mesh_instance.draw(light_projection * _light_view, shader);
    }
}

void grx_cascade_shadow_map_tech::bind_shadow_maps(int start) {
    //glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    std::iota(_gl_texture_nums.begin(), _gl_texture_nums.end(), start);
    for (auto [tex_num, texture_name] : zip_view(_gl_texture_nums, _shadow_maps)) {
        glBindTextureUnit(static_cast<GLuint>(tex_num), texture_name);
    }
}

void grx_cascade_shadow_map_tech::setup(const shared_ptr<grx_shader_program>& shader_program,
                                        const glm::mat4&                      object_matrix)
{
    auto found = _cached_uniforms.find(shader_program.get());
    if (found == _cached_uniforms.end())
        found = _cached_uniforms.emplace(shader_program.get(), *shader_program).first;

    vector<glm::mat4> light_mvps(_light_projections.size());
    for (auto& [light_mvp, light_proj] : zip_view(light_mvps, _light_projections))
        light_mvp = light_proj * _light_view * object_matrix;

    auto& u = found->second;
    u.light_mvps  = light_mvps;
    u.shadow_maps = _gl_texture_nums;
    u.csm_end_cs  = _csm_end_cs;
}

void grx_cascade_shadow_map_tech::setup_instanced(const shared_ptr<grx_shader_program>& shader_program) {
    auto found = _cached_uniforms.find(shader_program.get());
    if (found == _cached_uniforms.end())
        found = _cached_uniforms.emplace(shader_program.get(), *shader_program).first;

    vector<glm::mat4> light_mvps(_light_projections.size());
    for (auto& [light_mv, light_proj] : zip_view(light_mvps, _light_projections))
        light_mv = light_proj * _light_view;

    auto& u = found->second;
    u.light_mvps  = light_mvps;
    u.shadow_maps = _gl_texture_nums;
    u.csm_end_cs  = _csm_end_cs;
}

void grx_cascade_shadow_map_tech::culling_stage(const grx_camera& camera) const {
    grx::grx_frustum_mgr().calculate_culling(camera.extract_frustum(_z_bounds[0], _z_bounds[1] + _z_camera_shift, -_z_camera_shift),
                                             frustum_bits::csm_near);
    grx::grx_frustum_mgr().calculate_culling(camera.extract_frustum(_z_bounds[1], _z_bounds[2] + _z_camera_shift, -_z_camera_shift),
                                             frustum_bits::csm_middle);
    grx::grx_frustum_mgr().calculate_culling(camera.extract_frustum(_z_bounds[2], _z_bounds[3] + _z_camera_shift, -_z_camera_shift),
                                             frustum_bits::csm_far);
}

void grx_cascade_shadow_map_tech::shadow_path(core::span<grx_mesh_instance> objects,
                                              const grx_shader_tech&        sp) const
{
    auto bits = array{frustum_bits::csm_near, frustum_bits::csm_middle, frustum_bits::csm_far};

    for (auto& [shadow_map, light_projection, bit] : zip_view(_shadow_maps, _light_projections, bits)) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_map, 0);

        for (auto& object : objects)
            object.draw(light_projection * _light_view, sp, false, bit);
    }
}

void grx_cascade_shadow_map_tech::shadow_path(grx_mesh_pack&         mesh_pack,
                                              const grx_shader_tech& sp) const
{
    auto bits = array{frustum_bits::csm_near, frustum_bits::csm_middle, frustum_bits::csm_far};

    for (auto& [shadow_map, light_projection, bit] : zip_view(_shadow_maps, _light_projections, bits)) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadow_map, 0);
        mesh_pack.draw(light_projection * _light_view, sp, false, bit);
    }
}


} // namespace grx


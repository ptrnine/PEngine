#include "grx_cascaded_shadow_mapping_tech.hpp"
#include <core/types.hpp>
#include <core/container_extensions.hpp>
#include <core/assert.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <GL/glew.h>

using namespace core;

grx::grx_cascade_shadow_map_tech::grx_cascade_shadow_map_tech(
        const vec2u& size,
        size_t maps_count,
        const vector<float>& z_bounds
): _size(size), _shadow_maps(maps_count), _ortho_projections(maps_count), _z_bounds(z_bounds)
{
    RASSERT(z_bounds.size() == maps_count);

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
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, _shadow_maps[0], 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    auto rc = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    RASSERTF(rc == GL_FRAMEBUFFER_COMPLETE, "Shadow map framebuffer error: {}", rc);
}

grx::grx_cascade_shadow_map_tech::~grx_cascade_shadow_map_tech() {
    glDeleteFramebuffers(1, &_fbo);
    glDeleteTextures(static_cast<GLsizei>(_shadow_maps.size()), _shadow_maps.data());
}

void grx::grx_cascade_shadow_map_tech::set_z_bounds(const vector<float>& values) {
    RASSERT(static_cast<size_t>(values.size()) == _z_bounds.size());
    _z_bounds = values;
}

void grx::grx_cascade_shadow_map_tech::cover_view(const glm::mat4& view, float fov, const vec3f& dir) {
    auto inv_view   = glm::inverse(view);
    auto light_view = glm::lookAt({0.f, 0.f, 0.f}, to_glm(dir), {0.f, 1.f, 0.f});

    auto aspect_ratio   = static_cast<float>(_size.x()) / static_cast<float>(_size.y());
    auto tan_half_h_fov = glm::tan(glm::radians(fov * aspect_ratio / 2.f));
    auto tan_half_v_fov = glm::tan(glm::radians(fov / 2.f));

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
            auto lss = light_view * (inv_view * f);

            min.x() = std::min(min.x(), lss.x);
            max.x() = std::max(max.x(), lss.x);
            min.y() = std::min(min.y(), lss.y);
            max.y() = std::max(max.y(), lss.y);
            min.z() = std::min(min.z(), lss.z);
            max.z() = std::max(max.z(), lss.z);
        }

        _ortho_projections[i].left   = min.x();
        _ortho_projections[i].right  = max.x();
        _ortho_projections[i].bottom = min.y();
        _ortho_projections[i].top    = max.y();
        _ortho_projections[i].near   = min.z();
        _ortho_projections[i].far    = max.z();
    }
}


#include "grx_forward_renderer_light.hpp"
#include "algorithms/grx_frustum_culling.hpp"
#include "grx_utils.hpp"
#include "grx_mesh_instance.hpp"
#include "grx_camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

namespace grx {
using namespace core;

grx_dir_light_provider grx_forward_light_mgr::create_dir_light() {
    enable_dir_light_ = true;
    return grx_dir_light_provider(weak_from_this());
}

grx_point_light_provider grx_forward_light_mgr::create_point_light() {
    if (free_point_lights_.empty())
        throw std::runtime_error("Not free point lights!");

    auto num = free_point_lights_.back();
    free_point_lights_.pop_back();
    point_lights_idxs_.push_back(static_cast<int>(num));
    return grx_point_light_provider(weak_from_this(), num);
}

grx_spot_light_provider grx_forward_light_mgr::create_spot_light() {
    if (free_spot_lights_.empty())
        throw std::runtime_error("Not free spot lights!");

    auto num = free_spot_lights_.back();
    free_spot_lights_.pop_back();
    spot_lights_idxs_.push_back(static_cast<int>(num));
    return grx_spot_light_provider(weak_from_this(), num);
}

grx_fr_light_param_uniform& grx_fr_light_param_uniform::operator=(const grx_forward_light_mgr& mgr) {
    point_lights_count = static_cast<int>(mgr.point_lights_count());
    spot_lights_count  = static_cast<int>(mgr.spot_lights_count());
    point_lights_idxs  = mgr.point_lights_idxs();
    spot_lights_idxs   = mgr.spot_lights_idxs();
    specular_power     = mgr.specular_power();
    specular_intensity = mgr.specular_intensity();
    eye_pos_ws         = mgr.eye_position();
    dir_light_enabled  = mgr.has_dir_light() ? 1 : 0;

    return *this;
}

void grx_forward_light_mgr::setup_shadow_maps_to(core::shared_ptr<grx_shader_program>& shader) {
    csm_mgr_.bind_shadow_maps();

    auto found = cached_uniforms_.find(shader.get());

    if (found == cached_uniforms_.end())
        found = cached_uniforms_.emplace(shader.get(), uniforms_t(shader)).first;

    auto& u = found->second;
    u.param = *this;
    u.dir_light = dir_light_;

    for (int i : point_lights_idxs_)
        u.point_lights[static_cast<size_t>(i)] = point_lights_[static_cast<size_t>(i)]; // NOLINT

    int spot_shadow_map_idx = 5; // After two textures (diffuse:0, normals:1) and CSM textures (near:2, middle:3, far:4)
    shadow_map_mgr_.bind_shadow_map_textures(spot_shadow_map_idx);

    for (int i : index_seq(PE_FORWARD_MAX_SPOT_LIGHTS))
        u.spot_lights[static_cast<size_t>(i)].texture = spot_shadow_map_idx + i;

    for (int i : spot_lights_idxs_) {
        auto idx = static_cast<size_t>(i);
        u.spot_lights[idx] = spot_lights_[idx]; // NOLINT
    }
}

void grx_forward_light_mgr::setup_shadow_maps_to(grx_shader_tech& tech) {
    setup_shadow_maps_to(tech.base());
    setup_shadow_maps_to(tech.skeleton());
    setup_shadow_maps_to(tech.instanced());
}

void grx_forward_light_mgr::shadow_path(const shared_ptr<grx_camera>& camera,
                                        span<grx_mesh_instance>       objects,
                                        const grx_shader_tech&        shader_tech) {
    if (enable_dir_light_) {
        csm_mgr_.culling_stage(*camera);
        csm_mgr_.cover_view(camera, dir_light_.direction);
        csm_mgr_.bind_framebuffer();
        csm_mgr_.shadow_path(objects, shader_tech);
    }
    shadow_map_mgr_.shadow_path(objects, shader_tech, spot_lights_idxs_, spot_lights_);
    eye_position_ = camera->position();
}

void grx_forward_light_mgr::shadow_path(const shared_ptr<grx_camera>& camera,
                                        grx_mesh_pack&                mesh_pack,
                                        const grx_shader_tech&        shader_tech) {
    if (enable_dir_light_) {
        csm_mgr_.culling_stage(*camera);
        csm_mgr_.cover_view(camera, dir_light_.direction);
        csm_mgr_.bind_framebuffer();
        csm_mgr_.shadow_path(mesh_pack, shader_tech);
    }
    shadow_map_mgr_.shadow_path(mesh_pack, shader_tech, spot_lights_idxs_, spot_lights_);
    eye_position_ = camera->position();
}

grx_fr_shadow_map_mgr::grx_fr_shadow_map_mgr(vec2u size): size_(size) { // NOLINT
    glGenFramebuffers(1, &fbo_);
    glGenTextures(PE_FORWARD_MAX_SPOT_LIGHTS, spot_maps_.data());

    for (auto tex_name : spot_maps_) {
        glBindTexture(GL_TEXTURE_2D, tex_name);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32,
                static_cast<GLsizei>(size.x()), static_cast<GLsizei>(size.y()),
                0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    for (auto spot_map : spot_maps_)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, spot_map, 0);

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    auto rc = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    RASSERTF(rc == GL_FRAMEBUFFER_COMPLETE, "Shadow map framebuffer error: {}", rc);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void grx_fr_shadow_map_mgr::shadow_path(span<grx_mesh_instance> objects,
                                        const grx_shader_tech&  tech,
                                        span<const int>         spot_light_idxs,
                                        span<grx_spot_light>    spot_lights) const {
    glCullFace(GL_FRONT);
    glViewport(0, 0, static_cast<GLsizei>(size_.x()), static_cast<GLsizei>(size_.y()));
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    for (auto spot_light_idx : spot_light_idxs) {
        auto& spot_light = spot_lights[spot_light_idx];

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                spot_maps_[static_cast<size_t>(spot_light_idx)], 0);
        glClear(GL_DEPTH_BUFFER_BIT);

        grx_frustum_mgr().calculate_culling(grx_utils::extract_frustum(spot_light.VP),
                frustum_bits::spot_light << spot_light_idx);

        for (auto& obj : objects)
            obj.draw(spot_light.VP, tech, false, frustum_bits::spot_light << spot_light_idx);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCullFace(GL_BACK);
}

void grx_fr_shadow_map_mgr::shadow_path(grx_mesh_pack&         mesh_pack,
                                        const grx_shader_tech& tech,
                                        span<const int>        spot_light_idxs,
                                        span<grx_spot_light>   spot_lights) const {
    glCullFace(GL_FRONT);
    glViewport(0, 0, static_cast<GLsizei>(size_.x()), static_cast<GLsizei>(size_.y()));
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    for (auto spot_light_idx : spot_light_idxs) {
        auto& spot_light = spot_lights[spot_light_idx];

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                spot_maps_[static_cast<size_t>(spot_light_idx)], 0);
        glClear(GL_DEPTH_BUFFER_BIT);

        grx_frustum_mgr().calculate_culling(grx_utils::extract_frustum(spot_light.VP),
                frustum_bits::spot_light << spot_light_idx);

        mesh_pack.draw(spot_light.VP, tech, false, frustum_bits::spot_light << spot_light_idx);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCullFace(GL_BACK);
}

void grx_fr_shadow_map_mgr::bind_shadow_map_textures(int start_id) {
    for (auto idx : index_seq(PE_FORWARD_MAX_SPOT_LIGHTS))
        glBindTextureUnit(static_cast<GLuint>(start_id + idx), spot_maps_[static_cast<size_t>(idx)]);
}

void grx_spot_light::update_view_projection() {
    auto view = glm::lookAt(to_glm(position), to_glm(position + direction), glm::vec3(0.f, 1.f, 0.f));
    auto proj = glm::perspective(glm::degrees(acosf(cutoff) * 1.41421f), 1.f, 1.f, 200.f);
    VP = proj * view;
}

} // namespace grx

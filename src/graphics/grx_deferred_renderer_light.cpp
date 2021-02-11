#include "grx_deferred_renderer_light.hpp"
#include "grx_debug.hpp"

#include <GL/glew.h>

using namespace core;

namespace grx {

static float calc_ray_length(float di, float ac, float al, float aq, const vec3f& c) {
    constexpr float min_intensity = 1.f/256.f;
    float i = luminance(c) * di;
    return (-al + std::sqrt(al * al - 4.f * aq * (-i / min_intensity + ac))) / (2.f * aq);
}

grx_ds_light_param_uniform& grx_ds_light_param_uniform::operator=(const grx_ds_light_mgr& mgr) {
        specular_power     = mgr.specular_power();
        specular_intensity = mgr.specular_intensity();
        eye_pos_ws         = mgr.eye_position();
        return *this;
}

grx_ds_dir_light_provider grx_ds_light_mgr::create_dir_light() {
    enable_dir_light_ = true;
    return grx_ds_dir_light_provider(weak_from_this());
}

grx_ds_point_light_provider<> grx_ds_light_mgr::create_point_light() {
    point_lights_.emplace_back();
    return grx_ds_point_light_provider(weak_from_this(), --point_lights_.end());
}

grx_ds_spot_light_provider grx_ds_light_mgr::create_spot_light() {
    spot_lights_.emplace_back();
    return grx_ds_spot_light_provider(weak_from_this(), --spot_lights_.end());
}

grx_ds_point_light::grx_ds_point_light(): mesh_(gen_light_sphere()) {
    aabb_.min = {-1.f, -1.f, -1.f};
    aabb_.min = { 1.f,  1.f,  1.f};
}

grx_ds_spot_light::grx_ds_spot_light() {
    spot_shape.clear();
    uint pts_count = 32;
    float start = 180.f;
    float step  = 360.f / static_cast<float>(pts_count);
    for ([[maybe_unused]] auto _ : core::index_seq(pts_count)) {
        auto angle = core::angle::radian(start);
        spot_shape.emplace_back(vec2f{std::cos(angle), std::sin(angle)});
        start -= step;
    }
}

float grx_ds_point_light::max_ray_length() const {
    return calc_ray_length(
        diffuse_intensity, attenuation_constant, attenuation_linear, attenuation_quadratic, color);
}

void grx_ds_point_light::calc_model_matrix() {
    model_mat_ = glm::scale(glm::translate(glm::mat4(1.f), core::to_glm(position)),
                            glm::vec3(max_ray_length()));
    auto aabb = aabb_;
    aabb.transform(model_mat_);
    aabb_proxy_.aabb() = aabb;
}

void grx_ds_spot_light::calc_model_matrix() {
    float ray_len = max_ray_length();
    float pitch   = -std::asin(-direction.y());
    float yaw     = std::atan2(-direction.x(), -direction.z());

    glm::mat4 orientation = glm::rotate(glm::mat4(1.f), pitch, glm::vec3(1.f, 0.f, 0.f));
    orientation = glm::rotate(orientation, yaw, glm::vec3(0.f, 1.f, 0.f));

    model_mat_ =
        glm::rotate(glm::rotate(glm::scale(glm::translate(glm::mat4(1.f), to_glm(position)),
                                           glm::vec3(ray_len)),
                                yaw,
                                glm::vec3(0.f, 1.f, 0.f)),
                    pitch,
                    glm::vec3(1.f, 0.f, 0.f));

    auto aabb = aabb_;
    aabb.transform(model_mat_);
    aabb_proxy_.aabb() = aabb;
}

void grx_ds_spot_light::recalc_mesh() {
    float cut_sin = std::sqrt(1 - cutoff_ * cutoff_);
    float scale_xy_f = (cut_sin / cutoff_);

    aabb_ = grx_aabb::maximized();

    vector<vec3f> positions;
    positions.emplace_back(0.f, 0.f, 0.f);
    positions.emplace_back(0.f, 0.f, -1.f);

    vbo_vector_indices indices;
    for (auto& pos : spot_shape) {
        positions.emplace_back(pos * scale_xy_f, -1.f);
        aabb_.merge(grx_aabb{positions.back(), positions.back()});
    }
    aabb_.max.z() = 0.f;

    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(static_cast<uint>(positions.size() - 1));
    indices.push_back(1);
    indices.push_back(static_cast<uint>(positions.size() - 1));
    indices.push_back(2);

    for (uint idx : core::index_seq(2U, static_cast<uint>(positions.size() - 1))) {
        indices.push_back(0);
        indices.push_back(idx + 1);
        indices.push_back(idx);
        indices.push_back(1);
        indices.push_back(idx);
        indices.push_back(idx + 1);
    }

    mesh_.bind_vao();
    mesh_.set_data<0>(indices);
    mesh_.set_data<1>(positions);
}

void grx_ds_light_mgr::start_geometry_pass() {
    gbuf_.bind_geometry_pass();

    glDepthMask(GL_TRUE);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

static void stencil_path(grx_g_buffer& gbuf, light_mesh_t& light_mesh) {
    gbuf.bind_stencil_path();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClear(GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 0, 0);

    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR, GL_KEEP);

    light_mesh.bind_vao();
    light_mesh.draw(light_mesh.indices_count().value());
}

static void start_dir_light_pass(grx_g_buffer& gbuf) {
    gbuf.bind_lighting_pass();

    glDepthMask(GL_FALSE);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);
}

static void start_point_light_pass(grx_g_buffer& gbuf, light_mesh_t& light_mesh) {
    glDepthMask(GL_FALSE);

    stencil_path(gbuf, light_mesh);
    gbuf.bind_lighting_pass();

    glStencilFunc(GL_NOTEQUAL, 0, 0xFF);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
}

void grx_ds_light_mgr::light_pass(shared_ptr<grx_shader_program>& program, vec3f eye_pos, const glm::mat4& vp) {
    for (auto& light : point_lights_)
        light.calc_model_matrix();
    for (auto& light : spot_lights_)
        light.calc_model_matrix();

    gbuf_.clear_result_attachment();

    if (enable_debug_draw_) {
        for (auto& light : point_lights_) {
            if (light.aabb_proxy_.is_visible(frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far)) {
                grx_aabb_debug().draw(light.mesh(), vp, light.model_matrix(), light.color);
                grx_aabb_debug().push(light.aabb_, light.model_matrix(), color_rgb{0, 255, 127});
            }
        }
        for (auto& light : spot_lights_) {
            if (light.aabb_proxy_.is_visible(frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far)) {
                grx_aabb_debug().draw(light.mesh(), vp, light.model_matrix(), light.color);
                grx_aabb_debug().push(light.aabb_, light.model_matrix(), color_rgb{0, 255, 127});
            }
        }
    }

    auto found = cached_uniforms_.find(program.get());
    if (found == cached_uniforms_.end())
        cached_uniforms_.emplace(program.get(), uniforms_t(program));

    auto& uniforms = cached_uniforms_.find(program.get())->second;

    eye_position_ = eye_pos; // TODO: move this

    uniforms.param = *this;
    if (enable_dir_light_) {
        start_dir_light_pass(gbuf_);

        uniforms.param.set_overlay_mvp(glm::mat4(1.f));
        uniforms.dir_light = dir_light_;
        uniforms.param.light_type = grx_ds_light_type::directional;
        gbuf_.draw(program);
    }

    glEnable(GL_STENCIL_TEST);

    for (auto& light : point_lights_) {
        if (!light.aabb_proxy_.is_visible(frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far))
            continue;

        auto mvp = vp * light.model_matrix();
        sphere_program->get_uniform<glm::mat4>("MVP") = mvp;
        sphere_program->activate();
        start_point_light_pass(gbuf_, light.mesh());

        uniforms.param.set_overlay_mvp(mvp);
        uniforms.point_light = light;
        uniforms.param.light_type = grx_ds_light_type::point;
        gbuf_.draw(program, light.mesh());

        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
    }

    for (auto& light : spot_lights_) {
        if (!light.aabb_proxy_.is_visible(frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far))
            continue;

        auto mvp = vp * light.model_matrix();
        sphere_program->get_uniform<glm::mat4>("MVP") = mvp;
        sphere_program->activate();
        start_point_light_pass(gbuf_, light.mesh());

        uniforms.param.set_overlay_mvp(mvp);
        uniforms.spot_light = light;
        uniforms.param.light_type = grx_ds_light_type::spot;
        gbuf_.draw(program, light.mesh());

        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
    }

    glDisable(GL_STENCIL_TEST);
}

void grx_ds_light_mgr::light_pass(grx_shader_tech& tech, grx_mesh_type type, vec3f eye_pos, const glm::mat4& vp) {
    switch (type) {
        case grx_mesh_type::Basic:
            light_pass(tech.base(), eye_pos, vp);
            break;
        case grx_mesh_type::Skeleton:
            light_pass(tech.skeleton(), eye_pos, vp);
            break;
        case grx_mesh_type::Instanced:
            light_pass(tech.instanced(), eye_pos, vp);
            break;
    }
}

void grx_ds_light_mgr::present(vec2u out_size) {
    gbuf_.bind_present_pass();
    glBlitFramebuffer(0,
                      0,
                      static_cast<GLint>(gbuf_.fbo_size().x()),
                      static_cast<GLint>(gbuf_.fbo_size().y()),
                      0,
                      0,
                      static_cast<GLint>(out_size.x()),
                      static_cast<GLint>(out_size.y()),
                      GL_COLOR_BUFFER_BIT,
                      GL_LINEAR);
}

}

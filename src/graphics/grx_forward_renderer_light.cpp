#include "grx_forward_renderer_light.hpp"

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

    return *this;
}

void grx_forward_light_mgr::setup_to(core::shared_ptr<grx_shader_program>& shader) {
    auto found = cached_uniforms_.find(shader.get());

    if (found == cached_uniforms_.end())
        found = cached_uniforms_.emplace(shader.get(), uniforms_t(shader)).first;

    auto& u = found->second;
    u.param = *this;
    u.dir_light = dir_light_;

    for (int i : point_lights_idxs_)
        u.point_lights[static_cast<size_t>(i)] = point_lights_[static_cast<size_t>(i)]; // NOLINT

    for (int i : spot_lights_idxs_)
        u.spot_lights[static_cast<size_t>(i)] = spot_lights_[static_cast<size_t>(i)]; // NOLINT
}

} // namespace grx

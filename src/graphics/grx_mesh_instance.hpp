#pragma once

#include "algorithms/grx_frustum_culling.hpp"
#include "grx_debug.hpp"
#include "grx_mesh_mgr.hpp"
#include <core/helper_macros.hpp>
#include <core/log.hpp>
#include <core/types.hpp>

namespace grx
{
class grx_mesh_instance {
private:
    struct animation_spec {
        core::timer timer;
        bool        stop_on_end = true;
    };

    struct animation_data {
        core::optional<core::string>                 current_animation;
        core::hash_map<core::string, animation_spec> specs;
    };

public:
    grx_mesh_instance(grx_mesh_mgr& mesh_manager, core::string_view path): _mesh(mesh_manager.load(path, false)) {}

    void draw(const glm::mat4& view_projection, const grx_shader_tech& tech) {
        auto result_aabb = _mesh->aabb();

        if (auto& skeleton = _mesh->skeleton()) {
            if (_anim_data.current_animation) {
                auto& anim_name = _anim_data.current_animation.value();
                auto& anim      = skeleton->animations[anim_name];
                auto& spec      = _anim_data.specs[anim_name];
                auto  ticks     = spec.timer.measure_count() * anim.ticks_per_second;

                if (!spec.stop_on_end || ticks < anim.duration) {
                    ticks = std::fmod(ticks, anim.duration);
                    grx_mesh_mgr::anim_traverse(*skeleton, anim, ticks, skeleton->root_node.get());
                }
                else {
                    _anim_data.current_animation = std::nullopt;
                }
            }
            else {
                auto& transforms = skeleton->final_transforms;
                std::fill(transforms.begin(), transforms.end(), glm::mat4(1.f));
            }

            for (auto [bone_aabb, transform] : core::zip_view(skeleton->aabbs, skeleton->final_transforms)) {
                bone_aabb.transform(transform);
                result_aabb.merge(bone_aabb);

                if (_debug_bone_aabb_draw) {
                    constexpr auto color = vec{0.5f, 0.2f, 1.0f};
                    grx_aabb_debug().draw(bone_aabb, model_matrix(), view_projection, color);
                }
            }
        }

        result_aabb.transform(model_matrix());

        if (_debug_aabb_draw)
            grx_aabb_debug().draw(result_aabb, glm::mat4(1.f), view_projection, {0.f, 1.f, 0.f});

        _aabb_proxy.aabb() = result_aabb;

        if (_aabb_proxy.is_visible())
            _mesh->draw(view_projection, model_matrix(), tech);
    }

    void play_animation(const core::string& name, bool stop_on_end = true) {
        if (!has_skeleton())
            throw std::runtime_error("Can't play animation '" + name + "' : Mesh has no skeleton");

        if (!_mesh->skeleton()->animations.count(name))
            LOG_ERROR("Can't play animation '{}' : animation not found", name);

        _anim_data.current_animation = name;
        _anim_data.specs[name]       = animation_spec{core::timer(), stop_on_end};
    }

    [[nodiscard]]
    const grx_aabb& aabb() const {
        return _mesh->aabb();
    }

    [[nodiscard]]
    bool has_skeleton() const {
        return _mesh->skeleton().get() != nullptr;
    }

    void move(const core::vec3f& displacement) {
        _position += displacement;
    }

private:
    core::shared_ptr<grx_mesh> _mesh;
    core::vec3f                _position = {0.f, 0.f, 0.f};
    core::vec3f                _scale    = {1.f, 1.f, 1.f};
    glm::quat                  _rotation = {glm::vec3(0.f, 0.f, 0.f)};
    // glm::mat4                  _model_matrix = glm::mat4(1.f);
    animation_data         _anim_data;
    grx_aabb_culling_proxy _aabb_proxy;
    bool                   _debug_aabb_draw      = false;
    bool                   _debug_bone_aabb_draw = false;

public:
    DECLARE_GET_SET(position)
    DECLARE_GET_SET(scale)
    DECLARE_GET(rotation)
    DECLARE_VAL_GET_SET(debug_aabb_draw)
    DECLARE_VAL_GET_SET(debug_bone_aabb_draw)

    void set_rotation(const core::vec3f& degrees) {
        _rotation = glm::quat(glm::radians(core::to_glm(degrees)));
    }

    [[nodiscard]]
    glm::mat4 model_matrix() const {
        return glm::translate(glm::mat4(1.f), core::to_glm(_position)) * glm::mat4(_rotation) *
               glm::scale(glm::mat4(1.f), core::to_glm(_scale));
    }
};

class grx_mesh_pack {
public:
    grx_mesh_pack(grx_mesh_mgr& mesh_manager, core::string_view path, size_t start_count):
        _mesh(mesh_manager.load(path, true)), _matrices(start_count, glm::mat4(1.f)) {}

    void draw(const glm::mat4& view_projection, const grx_shader_tech& tech) {
        _mesh->draw_instanced(view_projection, _matrices, tech);
    }

    void set_position(size_t idx, const core::vec3f& position) {
        _matrices[idx][3].x = position.x(); // NOLINT
        _matrices[idx][3].y = position.y(); // NOLINT
        _matrices[idx][3].z = position.z(); // NOLINT
    }

    void move(size_t idx, const core::vec3f& displacement) {
        set_position(idx, position(idx) + displacement);
    }

    [[nodiscard]]
    core::vec3f position(size_t idx) const {
        return core::vec3f{_matrices[idx][3].x, _matrices[idx][3].y, _matrices[idx][3].z}; // NOLINT
    }

private:
    core::shared_ptr<grx_mesh> _mesh;
    core::vector<glm::mat4>    _matrices;
};

} // namespace grx


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
    grx_mesh_instance(grx_mesh_mgr& mesh_manager, core::string_view path): _mesh(mesh_manager.load(path, false)) {
        if (auto& skeleton = _mesh->skeleton())
            _bone_transforms = skeleton->final_transforms;
    }

    void _debug_draw(const glm::mat4& view_projection) {
        auto result_aabb = _mesh->aabb();

        if (auto& skeleton = _mesh->skeleton()) {
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
    }

    template <typename T>
        requires std::same_as<T, grx_shader_tech> ||
        std::same_as<T, core::shared_ptr<grx_shader_program>>
    void draw(const glm::mat4& view_projection,
              const T&         tech,
              bool             enable_textures = true,
              frustum_bits culling_bits = frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far)
    {
        if (_debug_aabb_draw || _debug_bone_aabb_draw)
            _debug_draw(view_projection);

        if (_aabb_proxy.is_visible(culling_bits)) {
            /* Top 10 anime poor optimizations TODO: do something about it */
            if (has_skeleton())
                _bone_transforms.swap(_mesh->skeleton()->final_transforms);

            _mesh->draw(view_projection, model_matrix(), tech, enable_textures);

            if (has_skeleton())
                _bone_transforms.swap(_mesh->skeleton()->final_transforms);
        }
    }

    template <typename T>
        requires std::same_as<T, grx_shader_tech> ||
        std::same_as<T, core::shared_ptr<grx_shader_program>>
    void draw(size_t           instances_count,
              const glm::mat4& view_projection,
              const T&         tech,
              bool             enable_textures = true,
              frustum_bits culling_bits = frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far)
    {
        if (_debug_aabb_draw || _debug_bone_aabb_draw)
            _debug_draw(view_projection);

        if (_aabb_proxy.is_visible(culling_bits)) {
            /* Top 10 anime poor optimizations TODO: do something about it */
            if (has_skeleton())
                _bone_transforms.swap(_mesh->skeleton()->final_transforms);

            _mesh->draw(instances_count, view_projection, model_matrix(), tech, enable_textures);

            if (has_skeleton())
                _bone_transforms.swap(_mesh->skeleton()->final_transforms);
        }
    }

    void update_animation_transforms() {
        auto result_aabb = _mesh->aabb();

        if (auto& skeleton = _mesh->skeleton()) {
            if (_anim_data.current_animation) {
                auto& anim_name = _anim_data.current_animation.value();
                auto& anim      = skeleton->animations[anim_name];
                auto& spec      = _anim_data.specs[anim_name];
                auto  ticks     = spec.timer.measure_count() * anim.ticks_per_second;

                if (!spec.stop_on_end || ticks < anim.duration) {
                    ticks = std::fmod(ticks, anim.duration);
                    grx_mesh_mgr::anim_traverse(_bone_transforms, *skeleton, anim, ticks, skeleton->root_node.get());
                }
                else if (_default_anim) {
                    play_animation(*_default_anim, false);
                }
                else {
                    _anim_data.current_animation = std::nullopt;
                }
            }
            else {
                std::fill(_bone_transforms.begin(), _bone_transforms.end(), glm::mat4(1.f));
            }

            for (auto [bone_aabb, transform] : core::zip_view(skeleton->aabbs, _bone_transforms)) {
                bone_aabb.transform(transform);
                result_aabb.merge(bone_aabb);
            }
        }

        result_aabb.transform(model_matrix());
        _aabb_proxy.aabb() = result_aabb;
    }

    void play_animation(const core::string& name, bool stop_on_end = true) {
        if (!has_skeleton())
            throw std::runtime_error("Can't play animation '" + name + "' : Mesh has no skeleton");

        if (!_mesh->skeleton()->animations.count(name))
            LOG_ERROR("Can't play animation '{}' : animation not found", name);

        _anim_data.current_animation = name;
        _anim_data.specs[name]       = animation_spec{core::timer(), stop_on_end};
    }

    void default_animation(core::string_view name) {
        _default_anim = name;
        if (has_skeleton() && !_anim_data.current_animation)
            play_animation(*_default_anim, false);
    }

    [[nodiscard]]
    core::optional<core::string> default_animation() const {
        return _default_anim;
    }

    void reset_default_animation() {
        if (has_skeleton() && _default_anim && _anim_data.current_animation == *_default_anim)
            _anim_data.current_animation = core::nullopt;
        _default_anim = core::nullopt;
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
    animation_data          _anim_data;
    core::vector<glm::mat4> _bone_transforms;
    grx_aabb_culling_proxy  _aabb_proxy;
    grx_shader_program*     _cached_program = nullptr;
    bool                    _debug_aabb_draw      = false;
    bool                    _debug_bone_aabb_draw = false;
    core::optional<core::string> _default_anim;

public:
    DECLARE_GET_SET(position)
    DECLARE_GET_SET(scale)
    DECLARE_GET(rotation)
    DECLARE_VAL_GET_SET(debug_aabb_draw)
    DECLARE_VAL_GET_SET(debug_bone_aabb_draw)

    [[nodiscard]]
    const grx_mesh& mesh() const {
        return *_mesh;
    }

    void set_rotation(const core::vec3f& degrees) {
        _rotation = glm::quat(glm::radians(core::to_glm(degrees)));
    }

    [[nodiscard]]
    glm::mat4 model_matrix() const {
        return glm::translate(glm::mat4(1.f), core::to_glm(_position)) * glm::mat4(_rotation) *
               glm::scale(glm::mat4(1.f), core::to_glm(_scale));
    }

    [[nodiscard]]
    grx_mesh_type type() const {
        return _mesh->type();
    }

    [[nodiscard]]
    bool is_visible(frustum_bits tested_bits = frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far) {
        return _aabb_proxy.is_visible(tested_bits);
    }
};

class grx_mesh_pack {
public:
    struct instance {
        [[nodiscard]]
        glm::mat4 calc_matrix() const {
            return glm::translate(glm::mat4(1.f), core::to_glm(position)) * glm::mat4(rotation) *
                   glm::scale(glm::mat4(1.f), core::to_glm(scale));
        }

        core::vec3f            position = {0.f, 0.f, 0.f};
        core::vec3f            scale    = {1.f, 1.f, 1.f};
        glm::quat              rotation = {glm::vec3(0.f, 0.f, 0.f)};
        grx_aabb_culling_proxy aabb_proxy;
        glm::mat4              model_matrix = glm::mat4(1.0f);
        bool                   debug_aabb_draw      = false;
        bool                   debug_bone_aabb_draw = false;
    };

    grx_mesh_pack(grx_mesh_mgr& mesh_manager, core::string_view path, size_t start_count):
        _mesh(mesh_manager.load(path, true)), _instances(start_count) {}

    void update_transforms() {
        for (auto& i : _instances) {
            auto result_aabb = _mesh->aabb();
            i.model_matrix = i.calc_matrix();
            result_aabb.transform(i.model_matrix);
            i.aabb_proxy.aabb() = result_aabb;
        }
    }


    template <typename T>
        requires std::same_as<T, grx_shader_tech> ||
        std::same_as<T, core::shared_ptr<grx_shader_program>>
    void draw(const glm::mat4& view_projection,
              const T&         tech,
              bool             enable_textures = true,
              frustum_bits culling_bits = frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far)
    {
        core::vector<glm::mat4> model_matrices;

        for (auto& i : _instances) {
            if (i.debug_aabb_draw) {
                /* TODO: debug draw */
            }

            if (i.aabb_proxy.is_visible(culling_bits))
                model_matrices.emplace_back(i.model_matrix);
        }

        _mesh->draw_instanced(view_projection, model_matrices, tech, enable_textures);
    }

    void position(size_t idx, const core::vec3f& position) {
        _instances[idx].position = position;
    }

    void move(size_t idx, const core::vec3f& displacement) {
        position(idx, position(idx) + displacement);
    }

    void set_rotation(size_t idx, const core::vec3f& degrees) {
        _instances[idx].rotation = glm::quat(glm::radians(core::to_glm(degrees)));
    }

    void scale(size_t idx, const core::vec3f& value) {
        _instances[idx].scale = value;
    }

    [[nodiscard]]
    const core::vec3f& position(size_t idx) const {
        return _instances[idx].position;
    }

    [[nodiscard]]
    size_t count() const {
        return _instances.size();
    }

private:
    core::shared_ptr<grx_mesh> _mesh;
    core::vector<instance>     _instances;
};

} // namespace grx


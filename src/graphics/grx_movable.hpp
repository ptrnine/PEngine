#pragma once

#include "core/helper_macros.hpp"
#include "graphics/grx_joint_animation.hpp"
#include "grx_types.hpp"

namespace grx
{
class grx_movable {
public:
    void move(const vec3f& displacement) {
        _position += displacement;
        recalc_mat();
    }

    void rotation_angles(const core::vec3f& degrees) {
        _rotation = glm::quat(glm::radians(core::to_glm(degrees)));
        recalc_mat();
    }

    void position(const core::vec3f& value) {
        _position = value;
        recalc_mat();
    }

    void scale(const core::vec3f& value) {
        _scale = value;
        recalc_mat();
    }

    grx_aabb update_aabb(const grx_aabb& aabb) {
        if (!_last_aabb)
            _last_aabb = aabb.get_transformed(_model_matrix);
        return *_last_aabb;
    }

    void direct_combine_transforms(std::initializer_list<glm::mat4> matrices) {
        recalc_mat();
        for (auto& mat : matrices)
            _model_matrix = glm::inverse(mat) * _model_matrix;
    }

    void setup_joint_animations(const core::shared_ptr<grx_joint_animation_holder>& anim_holder) {
        _joint_animator = std::make_unique<grx_joint_animation_player>();
        _joint_anim_holder = anim_holder;
    }

    void update_joint_animations() {
        if (_joint_anim_holder && _joint_animator)
            _joint_animator->update(*_joint_anim_holder);
    }

    [[nodiscard]]
    const core::unique_ptr<grx_joint_animation_player>& animation_player() const {
        return _joint_animator;
    }

private:
    void recalc_mat() {
        _last_aabb.reset();

        auto pos_shift = vec3f::filled_with(0.f);
        glm::quat rot_shift = glm::quat({0.f, 0.f, 0.f});

        if (_joint_animator) {
            pos_shift = _joint_animator->position();
            rot_shift = glm::quat(core::to_glm(_joint_animator->rotation()));
        }

        _model_matrix = glm::translate(glm::mat4(1.f), core::to_glm(_position + pos_shift)) *
                        glm::mat4(_rotation * rot_shift) * glm::scale(glm::mat4(1.f), core::to_glm(_scale));
    }

private:
    glm::mat4 _model_matrix{1.f};
    vec3f     _position{0.f, 0.f, 0.f};
    vec3f     _scale{1.f, 1.f, 1.f};
    glm::quat _rotation{glm::vec3{0.f, 0.f, 0.f}};
    core::optional<grx_aabb> _last_aabb;
    core::unique_ptr<grx_joint_animation_player> _joint_animator;
    core::shared_ptr<grx_joint_animation_holder> _joint_anim_holder;

public:
    DECLARE_GET(model_matrix)
    DECLARE_GET(position)
    DECLARE_GET(scale)
    DECLARE_GET(rotation)

    [[nodiscard]] bool need_update_aabb() const {
        return !_last_aabb;
    }
};

template <typename DerivedT>
class grx_instance_movable {
public:

private:

};
} // namespace grx

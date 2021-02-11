#pragma once

#include "core/helper_macros.hpp"
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

private:
    void recalc_mat() {
        _last_aabb.reset();
        _model_matrix = glm::translate(glm::mat4(1.f), core::to_glm(_position)) *
                        glm::mat4(_rotation) * glm::scale(glm::mat4(1.f), core::to_glm(_scale));
    }

private:
    glm::mat4 _model_matrix{1.f};
    vec3f     _position{0.f, 0.f, 0.f};
    vec3f     _scale{1.f, 1.f, 1.f};
    glm::quat _rotation{glm::vec3{0.f, 0.f, 0.f}};
    core::optional<grx_aabb> _last_aabb;

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

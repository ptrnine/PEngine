#include "grx_camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <core/math.hpp>

#include "graphics/grx_types.hpp"
#include "grx_window.hpp"
#include "grx_camera_manipulator.hpp"

using namespace core::angle;

void grx::grx_camera::look_at(const core::vec3f& pos) {
    if (!pos.essentially_equal(_pos, 0.001f)) {
        _dir   = (pos - _pos).normalize();
        _pitch = std::clamp   (asinf(-_dir.y()), -PITCH_LOCK, PITCH_LOCK);
        _yaw   = constraint_pi(-atan2f(-_dir.x(), -_dir.z()));
        _roll  = 0.0f;

        _orientation = glm::rotate(glm::mat4(1.f), _pitch, glm::vec3(1.f, 0.f, 0.f));
        _orientation = glm::rotate(_orientation,   _yaw,   glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 inv_orientation = glm::inverse(_orientation);

        _right       = core::from_glm(inv_orientation * glm::vec4(1.f, 0.f, 0.f, 1.f)).xyz();
        _up          = _right.cross(_dir);
        _orientation = glm::rotate(_orientation, _roll, to_glm(_dir));
    }
}

void grx::grx_camera::calc_orientation() {
    _orientation = glm::rotate(glm::mat4(1.f), _pitch, glm::vec3(1.f, 0.f, 0.f));
    _orientation = glm::rotate(_orientation,   _yaw,   glm::vec3(0.f, 1.f, 0.f));

    glm::mat4 inv_orient = glm::inverse(_orientation);

    _dir   = core::from_glm(inv_orient * glm::vec4(0.f, 0.f, -1.f, 1.f)).xyz();
    _right = core::from_glm(inv_orient * glm::vec4(1.f, 0.f,  0.f, 1.f)).xyz();

    _up          = _right.cross(_dir);
    _orientation = glm::rotate(_orientation, _roll, to_glm(_dir));
}

void grx::grx_camera::calc_view_projection() {
    _view        = _orientation * glm::translate(glm::mat4(1.f), to_glm(-_pos));
    _projection  = glm::perspective(glm::radians(_fov), _aspect_ratio, _z_near, _z_far);
}

void grx::grx_camera::update(grx_window* window) {
    bool wnd_test = window ? window->on_focus() : true;
    if (_camera_manipulator && wnd_test) {
        _camera_manipulator->update_start();
        _camera_manipulator->update_fov(window, _fov);
        _camera_manipulator->update_orientation(window, _yaw, _pitch, _roll);

        _yaw   = constraint_pi(_yaw);
        _fov   = std::clamp(_fov,    FOV_MIN,    FOV_MAX);
        _pitch = std::clamp(_pitch, -PITCH_LOCK, PITCH_LOCK);
        _roll  = std::clamp(_roll,  -ROLL_LOCK,  ROLL_LOCK);
    }

    calc_orientation();

    if (_camera_manipulator && wnd_test)
        _camera_manipulator->update_position(window, _pos, _dir, _right, _up);

    calc_view_projection();
}

auto extract_frustum_helper(const glm::mat4& vp) -> grx::grx_aabb_frustum_planes_fast {
    grx::grx_aabb_frustum_planes_fast planes; // NOLINT

    planes.as_names.left.x() = vp[0][3] + vp[0][0];
    planes.as_names.left.y() = vp[1][3] + vp[1][0];
    planes.as_names.left.z() = vp[2][3] + vp[2][0];
    planes.as_names.left.w() = vp[3][3] + vp[3][0];

    planes.as_names.right.x() = vp[0][3] - vp[0][0];
    planes.as_names.right.y() = vp[1][3] - vp[1][0];
    planes.as_names.right.z() = vp[2][3] - vp[2][0];
    planes.as_names.right.w() = vp[3][3] - vp[3][0];

    planes.as_names.bottom.x() = vp[0][3] + vp[0][1];
    planes.as_names.bottom.y() = vp[1][3] + vp[1][1];
    planes.as_names.bottom.z() = vp[2][3] + vp[2][1];
    planes.as_names.bottom.w() = vp[3][3] + vp[3][1];

    planes.as_names.top.x() = vp[0][3] - vp[0][1];
    planes.as_names.top.y() = vp[1][3] - vp[1][1];
    planes.as_names.top.z() = vp[2][3] - vp[2][1];
    planes.as_names.top.w() = vp[3][3] - vp[3][1];

    planes.as_names.near.x() = vp[0][3] + vp[0][2];
    planes.as_names.near.y() = vp[1][3] + vp[1][2];
    planes.as_names.near.z() = vp[2][3] + vp[2][2];
    planes.as_names.near.w() = vp[3][3] + vp[3][2];

    planes.as_names.far.x() = vp[0][3] - vp[0][2];
    planes.as_names.far.y() = vp[1][3] - vp[1][2];
    planes.as_names.far.z() = vp[2][3] - vp[2][2];
    planes.as_names.far.w() = vp[3][3] - vp[3][2];

    return planes;
}

auto grx::grx_camera::extract_frustum() const -> grx_aabb_frustum_planes_fast {
    return extract_frustum_helper(view_projection());
}

auto grx::grx_camera::extract_frustum(float z_near, float z_far) const -> grx_aabb_frustum_planes_fast {
    auto projection = glm::perspective(glm::radians(_fov), _aspect_ratio, z_near, z_far);
    return extract_frustum_helper(projection * _view);
}


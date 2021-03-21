#include "grx_camera.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <core/math.hpp>

#include "graphics/grx_types.hpp"
#include "grx_utils.hpp"
#include "grx_window.hpp"
#include "grx_camera_manipulator.hpp"

using namespace core::angle;

void grx::grx_camera::look_at(const core::vec3f& pos) {
    if (!pos.essentially_equal(_pos, 0.001f)) {
        _dir     = (pos - _pos).normalize();
        _ypr.y() = std::clamp(asinf(-_dir.y()), -PITCH_LOCK, PITCH_LOCK);
        _ypr.x() = constraint_pi(-atan2f(-_dir.x(), -_dir.z()));
        _ypr.z() = 0.0f;

        _orientation = glm::rotate(glm::mat4(1.f), _ypr.y(), glm::vec3(1.f, 0.f, 0.f));
        _orientation = glm::rotate(_orientation,   _ypr.x(),   glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 inv_orientation = glm::inverse(_orientation);

        _right       = core::from_glm(inv_orientation * glm::vec4(1.f, 0.f, 0.f, 1.f)).xyz();
        _up          = _right.cross(_dir);
        _orientation = glm::rotate(_orientation, _ypr.z(), to_glm(_dir));
    }
}

void grx::grx_camera::calc_orientation() {
    auto [yaw, pitch, roll] = ypr();

    _orientation = glm::rotate(glm::mat4(1.f), pitch, glm::vec3(1.f, 0.f, 0.f));
    _orientation = glm::rotate(_orientation,   yaw,   glm::vec3(0.f, 1.f, 0.f));

    glm::mat4 inv_orient = glm::inverse(_orientation);

    _dir   = core::from_glm(inv_orient * glm::vec4(0.f, 0.f, -1.f, 1.f)).xyz();
    _right = core::from_glm(inv_orient * glm::vec4(1.f, 0.f,  0.f, 1.f)).xyz();

    _up          = _right.cross(_dir);
    _orientation = glm::rotate(_orientation, roll, to_glm(_dir));
}

void grx::grx_camera::calc_view_projection() {
    _view        = _orientation * glm::translate(glm::mat4(1.f), to_glm(-position()));
    _projection  = glm::perspective(glm::radians(_fov), _aspect_ratio, _z_near, _z_far);
}

void grx::grx_camera::update(float timestep, grx_window* window) {
    _anim_player.update(*_anim_holder);

    bool wnd_test = window ? window->on_focus() : true;
    if (_camera_manipulator && wnd_test) {
        _camera_manipulator->update_fov(timestep, window, _fov);
        auto new_ypr = _ypr;
        _camera_manipulator->update_orientation(timestep, window, new_ypr);

        new_ypr.y() = std::clamp(new_ypr.y(), -PITCH_LOCK, PITCH_LOCK);
        new_ypr.z() = std::clamp(new_ypr.z(), -ROLL_LOCK, ROLL_LOCK);

        _ypr_speed = (new_ypr - _ypr) / timestep;
        _ypr = new_ypr;

        _ypr.x() = constraint_pi(_ypr.x());
        _fov     = std::clamp(_fov, FOV_MIN, FOV_MAX);
    }

    calc_orientation();

    if (_camera_manipulator && wnd_test)
        _camera_manipulator->update_position(timestep, window, _pos, _dir, _right, _up);

    calc_view_projection();
}

auto grx::grx_camera::extract_frustum() const -> grx_aabb_frustum_planes_fast {
    return grx_utils::extract_frustum(view_projection());
}

auto grx::grx_camera::extract_frustum(float z_near, float z_far, float z_shift) const -> grx_aabb_frustum_planes_fast {
    auto projection = glm::perspective(glm::radians(_fov), _aspect_ratio, z_near, z_far);
    auto view       = _orientation * glm::translate(glm::mat4(1.f), to_glm(-(_pos + _dir * z_shift)));
    return grx_utils::extract_frustum(projection * view);
}

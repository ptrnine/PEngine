#include "grx_camera_manipulator_fly.hpp"

#include <core/math.hpp>
#include <gainput/gainput.h>
#include <input/inp_input_ctx.hpp>
#include "core/container_extensions.hpp"
#include "grx_window.hpp"
#include "grx_camera.hpp"

namespace input_event {
    enum {
        RotateX,
        RotateY,
        MoveFront,
        MoveBack,
        MoveUp,
        MoveDown,
        MoveRight,
        MoveLeft,
        RollLeft,
        RollRight,
        Accel,
        FovIn,
        FovOut
    };
}

using namespace core;

core::shared_ptr<gainput::InputMap> grx::grx_camera_manipulator_fly::map_lock(grx_window* window) {
    if (window == nullptr)
        return {};

    if (window != _cached_wnd) {
        _input_map = window->create_input_map();

        auto mouse_id    = window->mouse_id();
        auto keyboard_id = window->keyboard_id();

        if (auto map = _input_map.lock()) {
            map->MapFloat (input_event::RotateX,   mouse_id,    gainput::MouseAxisX          );
            map->MapFloat (input_event::RotateY,   mouse_id,    gainput::MouseAxisY          );
            map->MapBool  (input_event::FovIn,     mouse_id,    gainput::MouseButtonWheelUp  );
            map->MapBool  (input_event::FovOut,    mouse_id,    gainput::MouseButtonWheelDown);
            map->MapBool  (input_event::MoveFront, mouse_id,    gainput::MouseButtonLeft     );
            map->MapBool  (input_event::MoveBack,  mouse_id,    gainput::MouseButtonRight    );
            map->MapBool  (input_event::MoveUp,    keyboard_id, gainput::KeyW                );
            map->MapBool  (input_event::MoveDown,  keyboard_id, gainput::KeyS                );
            map->MapBool  (input_event::MoveRight, keyboard_id, gainput::KeyD                );
            map->MapBool  (input_event::MoveLeft,  keyboard_id, gainput::KeyA                );
            map->MapBool  (input_event::Accel,     keyboard_id, gainput::KeyShiftL           );
            map->MapBool  (input_event::RollLeft,  keyboard_id, gainput::KeyQ                );
            map->MapBool  (input_event::RollRight, keyboard_id, gainput::KeyE                );
        }

        _cached_wnd = window;
    }

    return _input_map.lock();
}

void grx::grx_camera_manipulator_fly::update_fov(grx_window* wnd, float& fov) {
    if (auto map = map_lock(wnd)) {
        if (map->GetBool(input_event::FovIn))
            fov += 1.f;
        else if (map->GetBool(input_event::FovOut))
            fov -= 1.f;
    }
}

void grx::grx_camera_manipulator_fly::update_orientation(grx_window* window, float& yaw, float& pitch, float& roll) {
    if (auto map = map_lock(window)) {
        if (window->on_focus())
            window->reset_mouse_pos();

        auto pos = vec{map->GetFloat(input_event::RotateX), map->GetFloat(input_event::RotateY)};

        float zero = 0.f;
        if (!memcmp(&pos.x(), &zero, sizeof(float)))
            pos.x() = 0.5f;

        if (!memcmp(&pos.y(), &zero, sizeof(float)))
            pos.y() = 0.5f;

        auto mov = (pos - vec{0.5f, 0.5f}) * _mouse_speed;

        yaw   += mov.x();
        pitch += mov.y();

        // Roll
        auto rollSp = 4 * _timestep;

        roll = std::clamp(roll, -0.5f, 0.5f);

        if (map->GetBool(input_event::RollLeft))
            roll += rollSp;
        else if (map->GetBool(input_event::RollRight))
            roll -= rollSp;
        else if (memcmp(&roll, &zero, sizeof(float)) != 0) {
            if (roll > 0.f) {
                roll -= rollSp;
                if (roll < 0.f)
                    roll = 0.f;
            } else {
                roll += rollSp;
                if (roll > 0.f)
                    roll = 0.f;
            }
        }
    }
}

void grx::grx_camera_manipulator_fly::update_position(
        grx_window* wnd,
        core::vec3f& position,
        const core::vec3f& direction,
        const core::vec3f& right,
        const core::vec3f& up
) {
    if (auto map = map_lock(wnd)) {
        auto speed = map->GetBool(input_event::Accel) ? _speed * _shift_factor : _speed;

        if (map->GetBool(input_event::MoveFront))
            position += direction * _timestep * speed;

        if (map->GetBool(input_event::MoveBack))
            position -= direction * _timestep * speed;

        if (map->GetBool(input_event::MoveUp))
            position += up * _timestep * speed;

        if (map->GetBool(input_event::MoveDown))
            position -= up * _timestep * speed;

        if (map->GetBool(input_event::MoveRight))
            position += right * _timestep * speed;

        if (map->GetBool(input_event::MoveLeft))
            position -= right * _timestep * speed;
    }
}

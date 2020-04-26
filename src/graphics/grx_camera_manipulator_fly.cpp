#include "grx_camera_manipulator_fly.hpp"

#include <gainput/gainput.h>
#include <input/inp_input_ctx.hpp>
#include "grx_window.hpp"

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

grx::grx_camera_manipulator_fly::grx_camera_manipulator_fly(grx_window& window, float walk_speed, float mouse_speed):
        _speed(walk_speed), _mouse_speed(mouse_speed), _input_map(window.input_ctx().create_input_map())
{
    auto mouse_id    = window.input_ctx().mouse_id();
    auto keyboard_id = window.input_ctx().keyboard_id();

    _input_map.MapFloat (input_event::RotateX,   mouse_id,    gainput::MouseAxisX          );
    _input_map.MapFloat (input_event::RotateY,   mouse_id,    gainput::MouseAxisY          );
    _input_map.MapBool  (input_event::FovIn,     mouse_id,    gainput::MouseButtonWheelUp  );
    _input_map.MapBool  (input_event::FovOut,    mouse_id,    gainput::MouseButtonWheelDown);
    _input_map.MapBool  (input_event::MoveFront, mouse_id,    gainput::MouseButtonLeft     );
    _input_map.MapBool  (input_event::MoveBack,  mouse_id,    gainput::MouseButtonRight    );
    _input_map.MapBool  (input_event::MoveUp,    keyboard_id, gainput::KeyW                );
    _input_map.MapBool  (input_event::MoveDown,  keyboard_id, gainput::KeyS                );
    _input_map.MapBool  (input_event::MoveRight, keyboard_id, gainput::KeyD                );
    _input_map.MapBool  (input_event::MoveLeft,  keyboard_id, gainput::KeyA                );
    _input_map.MapBool  (input_event::Accel,     keyboard_id, gainput::KeyShiftL           );
    _input_map.MapBool  (input_event::RollLeft,  keyboard_id, gainput::KeyQ                );
    _input_map.MapBool  (input_event::RollRight, keyboard_id, gainput::KeyE                );
}

void grx::grx_camera_manipulator_fly::update_fov(grx_window*, float& fov) {
    if (_input_map.GetBool(input_event::FovIn))
        fov += 1.f;
    else if (_input_map.GetBool(input_event::FovOut))
        fov -= 1.f;
}

void grx::grx_camera_manipulator_fly::update_orientation(grx_window* window, float& yaw, float& pitch, float& roll) {
    if (window)
        window->reset_mouse_pos();

    auto x = _input_map.GetFloat(input_event::RotateX);
    auto y = _input_map.GetFloat(input_event::RotateY);

    // First frame bug :/
    float zero = 0.0f;
    if (!memcmp(&x, &zero, sizeof(float))) x = 0.5f;
    if (!memcmp(&y, &zero, sizeof(float))) y = 0.5f;

    yaw   -= _mouse_speed * (0.5f - x);
    pitch -= _mouse_speed * (0.5f - y);


    // Roll
    auto rollSp = 4 * _timestep;

    if (roll > 0.5f)
        roll = 0.5f;
    else if (roll < -0.5f)
        roll = -0.5f;

    if (_input_map.GetBool(input_event::RollLeft))
        roll += rollSp;
    else if (_input_map.GetBool(input_event::RollRight))
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

void grx::grx_camera_manipulator_fly::update_position(
        grx_window*,
        core::vec3f& position,
        const core::vec3f& direction,
        const core::vec3f& right,
        const core::vec3f& up
) {
    auto speed = _input_map.GetBool(input_event::Accel) ? _speed * _shift_factor : _speed;

    if (_input_map.GetBool(input_event::MoveFront))
        position += direction * _timestep * speed;

    if (_input_map.GetBool(input_event::MoveBack))
        position -= direction * _timestep * speed;

    if (_input_map.GetBool(input_event::MoveUp))
        position += up * _timestep * speed;

    if (_input_map.GetBool(input_event::MoveDown))
        position -= up * _timestep * speed;

    if (_input_map.GetBool(input_event::MoveRight))
        position += right * _timestep * speed;

    if (_input_map.GetBool(input_event::MoveLeft))
        position -= right * _timestep * speed;
}
#pragma once

#include <core/helper_macros.hpp>
#include <gainput/gainput.h>
#include "grx_camera_manipulator.hpp"

namespace grx {
    class grx_camera_manipulator_fly : public grx_camera_manipulator {
    public:
        explicit grx_camera_manipulator_fly(class grx_window& window, float speed = 4.f, float mouse_speed = 2.f);

    protected:
        void update_fov(class grx_window* window, float& fov) override;
        void update_orientation(class grx_window* window, float& yaw, float& pitch, float& roll) override;
        void update_position(
                class grx_window* window,
                core::vec3f& position,
                const core::vec3f& direction,
                const core::vec3f& right,
                const core::vec3f& up) override;

        float _shift_factor = 4.f;
        float _speed =  0.f;
        float _mouse_speed;

        gainput::InputMap _input_map;

    public:
        DECLARE_SET(speed)
        DECLARE_SET(mouse_speed)
        DECLARE_VAL_GET(speed)
        DECLARE_VAL_GET(mouse_speed)
    };

} // namespace grx

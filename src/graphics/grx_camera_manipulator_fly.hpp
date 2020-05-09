#pragma once

#include <core/helper_macros.hpp>
#include <core/data_structures/mem_vector.hpp>
#include <core/time.hpp>
#include <gainput/gainput.h>
#include "grx_camera_manipulator.hpp"

namespace grx {
    class grx_camera_manipulator_fly : public grx_camera_manipulator {
    public:
        static constexpr float outline_push = 0.01f;
        static constexpr float low_push     = outline_push;
        static constexpr float high_push    = 1.f - low_push;
        static constexpr float diff_push    = high_push - low_push - outline_push;


        explicit grx_camera_manipulator_fly(float speed = 4.f, float mouse_speed = 4.f):
            _speed(speed), _mouse_speed(mouse_speed) {}

    protected:
        core::shared_ptr<gainput::InputMap> map_lock(class grx_window* window);

        void update_fov(class grx_window* window, float& fov) override;
        void update_orientation(class grx_window* window, float& yaw, float& pitch, float& roll) override;
        void update_position(
                class grx_window* window,
                core::vec3f& position,
                const core::vec3f& direction,
                const core::vec3f& right,
                const core::vec3f& up) override;

        float _shift_factor = 10.f;
        float _speed =  0.f;
        float _mouse_speed;

        core::weak_ptr<gainput::InputMap> _input_map;
        class grx_window* _cached_wnd = nullptr;

    public:
        DECLARE_SET(speed)
        DECLARE_SET(mouse_speed)
        DECLARE_VAL_GET(speed)
        DECLARE_VAL_GET(mouse_speed)
    };

} // namespace grx

#pragma once

#include <core/vec.hpp>

namespace grx {
    /**
     * Define the interface for classes that can control cameras
     */
    class grx_camera_manipulator {
    public:
        grx_camera_manipulator();
        virtual ~grx_camera_manipulator() = default;

    protected:
        friend class grx_camera;

        void update_start();

        virtual void update_fov(class grx_window* window, float& fov) = 0;
        virtual void update_orientation(class grx_window* window, float& yaw, float& pitch, float& roll) = 0;
        virtual void update_position(
                class grx_window* window,
                core::vec3f& position,
                const core::vec3f& direction,
                const core::vec3f& right,
                const core::vec3f& up) = 0;


        double _last_update_time;
        float  _timestep = 0.0;
    };

} // namespace grx
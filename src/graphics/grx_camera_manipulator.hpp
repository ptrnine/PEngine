#pragma once

#include <core/vec.hpp>
#include <core/time.hpp>

namespace grx {
    /**
     * Define the interface for classes that can control cameras
     */
    class grx_camera_manipulator {
    public:
        virtual ~grx_camera_manipulator() = default;

    protected:
        friend class grx_camera;

        virtual void update_fov(float timestep, class grx_window* window, float& fov) = 0;
        virtual void update_orientation(
            float timestep, class grx_window* window, core::vec3f& ypr) = 0;
        virtual void update_position(float              timestep,
                                     class grx_window*  window,
                                     core::vec3f&       position,
                                     const core::vec3f& direction,
                                     const core::vec3f& right,
                                     const core::vec3f& up)                                  = 0;
    };

} // namespace grx

#include "grx_camera_manipulator.hpp"

#include <GLFW/glfw3.h>

void grx::grx_camera_manipulator::update_start() {
    _timestep_d = _timer.tick_count<double>();
    _timestep   = static_cast<float>(_timestep_d);
}

#include "grx_camera_manipulator.hpp"

#include <GLFW/glfw3.h>

grx::grx_camera_manipulator::grx_camera_manipulator(): _last_update_time(glfwGetTime()) {}

void grx::grx_camera_manipulator::update_start() {
    auto current_time = glfwGetTime();
    _timestep         = static_cast<float>(current_time - _last_update_time);
    _last_update_time = current_time;
}
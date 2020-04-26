#include "inp_input_ctx.hpp"

#include <X11/Xlib.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>


void inp::inp_input_ctx::set_glfw_wnd(GLFWwindow* wnd) {
    x11_display = glfwGetX11Display();
    _wnd = wnd;
}

void inp::inp_input_ctx::update() {
    manager.Update();

    std::vector<XEvent> glfw_events;
    XEvent event;

    while (XPending(x11_display)) {
        XNextEvent(x11_display, &event);
        manager.HandleEvent(event);
        glfw_events.push_back(event);
    }

    // Put back events for glfw :/
    for (auto& e : glfw_events)
        XPutBackEvent(x11_display, &e);

    glfwPollEvents();
}

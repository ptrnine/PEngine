#include "inp_input_ctx.hpp"

#include <X11/Xlib.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_X11
#include <GLFW/glfw3native.h>

#include "inp_unix_mouse.hpp"

void inp::inp_input_ctx::update() {
    auto x11_display = glfwGetX11Display();

    /*
     * Gardabage collector for destroyed windows
     */
    core::vector<GLFWwindow*> to_delete;
    for (auto& [glfw_window, state] : _window_map)
        if (state.mgr.use_count() == 1)
            to_delete.push_back(glfw_window);

    for (auto glfw_window : to_delete)
        _window_map.erase(glfw_window);

    /*
     * Perform input
     */
    for (auto& [wnd, state] : _window_map) {
        if (wnd) {
            auto x11_wnd = glfwGetX11Window(wnd);

            if (auto pos = state.state.mouse_pos.try_get_and_deactivate())
                XWarpPointer(x11_display, None, x11_wnd, 0, 0, 0, 0, pos->x(), pos->y());
        }
    }

    /*
     * Handle all events
     */
    core::vector<XEvent> glfw_events;
    XEvent event;

    while (XPending(x11_display)) {
        XNextEvent(x11_display, &event);

        for (auto& [wnd, state] : _window_map) {
            if (wnd) {
                auto x11_wnd = glfwGetX11Window(wnd);

                /*
                 * Handle window events
                 */
                switch (event.type) {
                    case FocusIn:
                        if (event.xfocus.window == x11_wnd)
                            state.state.on_focus = true;
                        break;
                    case FocusOut:
                        if (event.xfocus.window == x11_wnd)
                            state.state.on_focus = false;
                        break;
                }
            }

            state.mgr->HandleEvent(event);
        }

        glfw_events.push_back(event);
    }

    if (_raw_mouse_input)
        _raw_mouse_input->update_positions();

    /*
     * Put back events for glfwPollEvents
     * (Shitty solution, but I have no any ideas)
     */
    for (auto& e : glfw_events)
        XPutBackEvent(x11_display, &e);

    glfwPollEvents();
}

core::vec2f inp::inp_input_ctx::raw_mouse_movement(mouse_id id) const {
    if (!_raw_mouse_input)
        throw std::runtime_error("There is no raw mouse input context!");

    return _raw_mouse_input->get_last_position(id);
}

void inp::inp_input_ctx::enable_default_raw_mouse_input() {
    _raw_mouse_input = std::make_unique<std::remove_pointer_t<decltype(_raw_mouse_input.get())>>();
}

void inp::inp_input_ctx::disable_raw_mouse_input() {
    _raw_mouse_input = nullptr;
}

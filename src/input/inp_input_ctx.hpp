#pragma once

#include <core/types.hpp>
#include <core/vec.hpp>

#include <gainput/gainput.h>

typedef struct _XDisplay Display;
typedef struct GLFWwindow GLFWwindow;

namespace inp {
    class inp_input_ctx {
    public:
        void set_glfw_wnd(GLFWwindow* wnd);

        void set_display_size(const core::vec2i& size) {
            manager.SetDisplaySize(size.x(), size.y());
        }

        auto create_input_map() {
            return gainput::InputMap(manager);
        }

        [[nodiscard]]
        gainput::DeviceId mouse_id   () const { return _mouse_id; }

        [[nodiscard]]
        gainput::DeviceId keyboard_id() const { return _keyboard_id; }

        [[nodiscard]]
        gainput::DeviceId gamepad_id () const { return _gamepad_id; }

        void update();

    protected:
        GLFWwindow* _wnd = nullptr;
        Display* x11_display = nullptr;

        gainput::InputManager manager;
        const gainput::DeviceId _mouse_id    = manager.CreateDevice<gainput::InputDeviceMouse>();
        const gainput::DeviceId _keyboard_id = manager.CreateDevice<gainput::InputDeviceKeyboard>();
        const gainput::DeviceId _gamepad_id  = manager.CreateDevice<gainput::InputDevicePad>();
    };
}

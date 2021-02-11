#include "core/main.cpp"
#include "core/log.hpp"
#include "input/inp_linux.hpp"

using namespace core;

int pe_main(args_view args) {
    args.require_end();

    auto def_kbd = inp::inp_linux::default_keyboard_device();
    auto def_mouse = inp::inp_linux::default_mouse_device();
    LOG("Default kbd: {}", def_kbd);
    LOG("Default mouse: {}", def_mouse);

    auto r = inp::input().create_receiver(def_kbd, def_mouse);

    LOG("Receiver start");
    while (true) {
        {
            auto t = r->start_transaction();
            if (t.is_pressed(inp::key::Z))
                LOG("Z key pressed");
            if (t.is_released(inp::key::Z))
                LOG("Z key released");

            auto xy = vec{t.trackpos(inp::track::mouse_x),
                          t.trackpos(inp::track::mouse_y)};
            if (xy != vec<i16, 2>{0, 0})
                LOG("Mouse move: {}", xy);
        }
    }

    return 0;
}

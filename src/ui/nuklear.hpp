#pragma once

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_KEYSTATE_BASED_INPUT

#include <nuklear.h>
#include "nuklear_glfw_gl3.h"

#include <core/helper_macros.hpp>
#include <core/types.hpp>
#include <graphics/grx_window.hpp>

#include "nuklear_binding.hpp"

#define UI_NK_MAX_VERTEX_BUFFER 512 * 1024
#define UI_NK_MAX_ELEMENT_BUFFER 128 * 1024

namespace ui {
class ui_nk_ctx {
    SINGLETON_IMPL(ui_nk_ctx);

private:
    ui_nk_ctx() = default;
    ~ui_nk_ctx() = default;

public:
    struct ctx_state {
        nk_glfw glfw;
        nk_font_atlas* atlas;
        nk_context* ctx;
    };

    void init_for(const grx::grx_window& window) {
        auto [pos, was_inserted] = _contexts.emplace(window.glfw_window(), ctx_state{});
        if (was_inserted) {
            pos->second.ctx = nk_glfw3_init(&pos->second.glfw, pos->first, NK_GLFW3_INSTALL_CALLBACKS);
            nk_glfw3_font_stash_begin(&pos->second.glfw, &pos->second.atlas);
            nk_glfw3_font_stash_end(&pos->second.glfw);
            nk_style_load_all_cursors(pos->second.ctx, pos->second.atlas->cursors);
        }
    }

    void deinit_for(const grx::grx_window& window) {
        nk_glfw3_shutdown(&_contexts.at(window.glfw_window()).glfw);
        _contexts.erase(window.glfw_window());
    }

    void new_frame(const grx::grx_window& window) {
        nk_glfw3_new_frame(&_contexts.at(window.glfw_window()).glfw);
    }

    void render(const grx::grx_window& window) {
        nk_glfw3_render(&_contexts.at(window.glfw_window()).glfw,
                        NK_ANTI_ALIASING_ON,
                        UI_NK_MAX_VERTEX_BUFFER,
                        UI_NK_MAX_ELEMENT_BUFFER);
    }

    [[nodiscard]]
    nk_context* nk_ctx(const grx::grx_window& window) const {
        return _contexts.at(window.glfw_window()).ctx;
    }

private:
    core::map<GLFWwindow*, ctx_state> _contexts;
};

inline ui_nk_ctx& ctx() {
    return ui_nk_ctx::instance();
}

class ui_ctx : public ui_nuklear_base {
public:
    ui_ctx(core::shared_ptr<grx::grx_window> window): ui_nuklear_base(), _wnd(std::move(window)) {
        ctx().init_for(*_wnd);
        _ctx = ctx().nk_ctx(*_wnd);
    }

    ~ui_ctx() {
        if (_wnd)
            ctx().deinit_for(*_wnd);
    }

    ui_ctx(ui_ctx&&) = default;
    ui_ctx& operator=(ui_ctx&&) = default;

    ui_ctx(const ui_ctx&) = delete;
    ui_ctx& operator=(const ui_ctx&) = delete;

    void new_frame() {
        ctx().new_frame(*_wnd);
    }

    void render() {
        ctx().render(*_wnd);
    }

    [[nodiscard]]
    nk_context* nk_ctx() const {
        return ctx().nk_ctx(*_wnd);
    }

private:
    core::shared_ptr<grx::grx_window> _wnd;
};

}

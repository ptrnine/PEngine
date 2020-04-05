#pragma once
#include <thread>
#include <core/helper_macros.hpp>

namespace grx {
    // Holds global OpenGL state
    class grx_context {
        SINGLETON_IMPL(grx_context);

        void bind_default_framebuffer();

    private:
        grx_context();
    };

    inline grx_context& grx_ctx() {
        return grx_context::instance();
    }
} // namespace grx

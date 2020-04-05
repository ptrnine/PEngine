#pragma once
#include <core/types.hpp>
#include <core/vec.hpp>
#include "grx_types.hpp"
#include "grx_render_target.hpp"

struct GLFWwindow;

namespace core {
    class config_manager;
}

namespace grx {
    class grx_shader_manager;

    class grx_window {
    public:
        grx_window(
                const core::string& name,
                const core::vec2i& size,
                grx_shader_manager& shader_manager,
                core::config_manager& config_manager);
        ~grx_window();

        void make_current();
        bool should_close();
        void poll_events();
        void swap_buffers();
        void present();
        void bind_renderer();

        //[[nodiscard]]
        //grx_render_target& render_target() {
        //    return _render_target;
        //}

        [[nodiscard]]
        uint screen_quad_vbo() const {
            return _screen_quad_vbo;
        }

    private:
        GLFWwindow*          _wnd;
        uint                 _screen_quad_vbo;
        grx_render_target    _render_target;
        shader_program_id_t  _screen_quad_passthrough;
        uniform_id_t         _screen_quad_texture_uniform;
    };
} // namespace grx

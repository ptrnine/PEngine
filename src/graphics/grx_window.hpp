#pragma once
#include <core/types.hpp>
#include <core/vec.hpp>
#include <core/time.hpp>
#include <input/inp_input_ctx.hpp>

#include "grx_types.hpp"
#include "grx_postprocess_mgr.hpp"
#include "grx_vbo_tuple.hpp"

struct GLFWwindow;

namespace core {
    class config_manager;
}

namespace grx {
    class grx_shader_mgr;

    using grx_window_render_target =
            grx_postprocess_mgr<
                grx_render_target_settings<grx::grx_color_fmt::RGB, grx::grx_filtering::Nearest>>;

    class grx_window {
    public:
        grx_window(
                const core::string& name,
                const core::vec2i& size,
                grx_shader_mgr& shader_manager,
                core::config_manager& config_manager);
        ~grx_window();

        void make_current();
        bool should_close();
        void poll_events();
        void swap_buffers();
        void present();
        void bind_render_target();
        void bind_and_clear_render_target();

        void push_postprocess(const grx_postprocess& postprocess) {
            _render_target.push(postprocess);
        }

        void update_input();

        void attach_camera(core::shared_ptr<class grx_camera> camera) {
            _camera = std::move(camera);
        }

        //[[nodiscard]]
        //grx_render_target_tuple& render_target() {
        //    return _render_target;
        //}

        //[[nodiscard]]
        //uint screen_quad_vbo() const {
        //    return _screen_quad_vbo;
        //}

        inp::inp_input_ctx& input_ctx() {
            return _input_ctx;
        }

        [[nodiscard]]
        const inp::inp_input_ctx& input_ctx() const {
            return _input_ctx;
        }

        [[nodiscard]]
        const core::shared_ptr<class grx_camera>& camera() const {
            return _camera;
        }

        [[nodiscard]]
        core::shared_ptr<class grx_camera>& camera() {
            return _camera;
        }

        [[nodiscard]]
        core::vec2i size() const;

        [[nodiscard]]
        bool on_focus() const {
            return _on_focus;
        }

        void set_mouse_pos(const core::vec2f& position);
        void reset_mouse_pos();

    private:
        static void glfw_focus_callback(GLFWwindow* window, int focused);

    private:
        GLFWwindow*                     _wnd;
        grx_vbo_tuple<vbo_vector_vec3f> _vbo_tuple;
        grx_window_render_target        _render_target;
        shader_program_id_t             _screen_quad_passthrough;
        uniform_id_t                    _screen_quad_texture_uniform;

        inp::inp_input_ctx                 _input_ctx;
        core::shared_ptr<class grx_camera> _camera;

        bool _on_focus;
    };
} // namespace grx
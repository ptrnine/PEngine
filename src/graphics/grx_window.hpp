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
    using grx_window_render_target =
            grx_postprocess_mgr<
                grx_render_target_settings<grx::grx_color_fmt::RGB, grx::grx_filtering::Nearest>>;

    class grx_window {
    public:
        grx_window(
                const core::string& name,
                const core::vec2u& size,
                const core::shared_ptr<grx_window_render_target>& render_target = nullptr);
        ~grx_window();

        void make_current();
        bool should_close();
        void poll_events();
        void swap_buffers();
        void present();
        void bind_render_target();
        void bind_and_clear_render_target();

        void push_postprocess(const grx_postprocess& postprocess) {
            _render_target->push(postprocess);
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

        [[nodiscard]]
        const core::shared_ptr<class grx_camera>& camera() const {
            return _camera;
        }

        [[nodiscard]]
        core::shared_ptr<class grx_camera>& camera() {
            return _camera;
        }

        [[nodiscard]]
        core::vec2u size() const;

        [[nodiscard]]
        bool on_focus() const {
            return inp::inp_ctx().window_state_for(_wnd).on_focus;
        }

        [[nodiscard]]
        gainput::InputManager& input_mgr() {
            return *_input_mgr;
        }

        [[nodiscard]]
        const gainput::InputManager& input_mgr() const {
            return *_input_mgr;
        }

        [[nodiscard]]
        core::weak_ptr<gainput::InputMap> create_input_map() {
            return inp::inp_ctx().create_input_map(_wnd);
        }

        void set_mouse_pos(const core::vec2f& position);
        void reset_mouse_pos();

        void set_pos(const core::vec2i& position);

    private:
        GLFWwindow*                           _wnd;
        core::shared_ptr<grx_shader_program>  _screen_quad_passthrough;
        grx_uniform<int>                      _screen_quad_texture;
        //shader_program_id_t _screen_quad_passthrough;
        //uniform_id_t                    _screen_quad_texture_uniform;

        core::shared_ptr<grx_window_render_target> _render_target;
        core::shared_ptr<gainput::InputManager>    _input_mgr;
        core::shared_ptr<class grx_camera>         _camera;

        uint _mouse_id;
        uint _keyboard_id;

    public:
        DECLARE_GET          (render_target)
        DECLARE_VAL_GET      (mouse_id)
        DECLARE_VAL_GET      (keyboard_id)
        DECLARE_NON_CONST_GET(render_target)
    };
} // namespace grx

#pragma once

#include <core/vec.hpp>
#include "grx_render_target.hpp"
#include "grx_vbo_tuple.hpp"
#include "grx_shader_mgr.hpp"
#include "grx_postprocess.hpp"

namespace grx {
    template <typename T>
    class grx_postprocess_mgr {
        static_assert(RenderTargetSettings<T>);

    protected:
        friend class grx_window;
        grx_postprocess_mgr(): _vbo_tuple(nullptr) {}

    public:
        grx_postprocess_mgr(const core::vec2i& size): _render_target_tuple(size) {
            _vbo_tuple.set_data<0>({
                {-1.0f, -1.0f, 0.0f },
                { 1.0f, -1.0f, 0.0f },
                {-1.0f,  1.0f, 0.0f },
                {-1.0f,  1.0f, 0.0f },
                { 1.0f, -1.0f, 0.0f },
                { 1.0f,  1.0f, 0.0f }
            });
        }

        void bind_render_target() {
            _render_target_tuple.template bind<0>();
            _render_target_tuple.template swap_targets<0, 1>();
        }

        void bind_and_clear_render_target() {
            _render_target_tuple.template bind_and_clear<0>();
            _render_target_tuple.template swap_targets<0, 1>();
        }

        void step(shader_program_id_t shader_program, uniform_id_t texture_uniform) {
            _render_target_tuple.template bind_and_clear<0>();

            _vbo_tuple.bind_vao();
            _render_target_tuple.template activate_texture<1>();
            grx::grx_shader_mgr::use_program(shader_program);
            grx::grx_shader_mgr::set_uniform(texture_uniform, 0); // Texture position

            _vbo_tuple.draw(18);

            _render_target_tuple.template swap_targets<0, 1>();
        }

        void step(const grx_postprocess& postprocess) {
            _render_target_tuple.template bind_and_clear<0>();

            _vbo_tuple.bind_vao();
            _render_target_tuple.template activate_texture<1>();
            postprocess.bind_program_and_uniforms();

            _vbo_tuple.draw(18);

            _render_target_tuple.template swap_targets<0, 1>();
        }

        void activate_texture() {
            _render_target_tuple.template activate_texture<1>();
        }

        void push(const grx_postprocess& postprocess) {
            _postprocesses.push_back(postprocess);
        }

        void do_postprocess_queue() {
            for (auto& p : _postprocesses)
                step(p);
        }

    private:
        grx_vbo_tuple<vbo_vector_vec3f> _vbo_tuple;
        grx_render_target_tuple<T, T>   _render_target_tuple;
        core::vector<grx_postprocess>   _postprocesses;
    };
}
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

    public:
        static core::shared_ptr<grx_postprocess_mgr<T>>
        create_shared(const core::vec2u& size) {
            return core::make_shared<grx_postprocess_mgr>(size);
        }

        grx_postprocess_mgr(const core::vec2u& size): _render_target_tuple(size) {
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

        void step(grx_postprocess& postprocess) {
            _render_target_tuple.template bind_and_clear<0>();

            _vbo_tuple.bind_vao();
            _render_target_tuple.template activate_texture<1>();
            postprocess.bind();

            draw_quad();

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

        void bind_quad_vao() {
            _vbo_tuple.bind_vao();
        }

        void draw_quad() {
            constexpr size_t screen_vertices_count = 18;
            _vbo_tuple.draw(screen_vertices_count);
        }

    private:
        grx_vbo_tuple<vbo_vector_vec3f> _vbo_tuple;
        grx_render_target_tuple<T, T>   _render_target_tuple;
        core::vector<grx_postprocess>   _postprocesses;
    };
}

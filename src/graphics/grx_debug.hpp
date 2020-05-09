#pragma once

#include "grx_vbo_tuple.hpp"

#include <core/helper_macros.hpp>
#include <core/config_manager.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "grx_types.hpp"
#include "grx_context.hpp"
#include "grx_shader_mgr.hpp"

namespace grx {

    class grx_aabb_debug_drawer {
        SINGLETON_IMPL(grx_aabb_debug_drawer);

    private:
        ~grx_aabb_debug_drawer() = default;
        grx_aabb_debug_drawer() {
            _box.set_data<0>({
                { 1.f, -1.f, -1.f},
                {-1.f, -1.f, -1.f},
                {-1.f,  1.f, -1.f},
                { 1.f, -1.f, -1.f},
                {-1.f,  1.f, -1.f},
                { 1.f,  1.f, -1.f},

                { 1.f, -1.f,  1.f},
                { 1.f, -1.f, -1.f},
                { 1.f,  1.f, -1.f},
                { 1.f, -1.f,  1.f},
                { 1.f,  1.f, -1.f},
                { 1.f,  1.f,  1.f},

                {-1.f, -1.f,  1.f},
                { 1.f, -1.f,  1.f},
                { 1.f,  1.f,  1.f},
                {-1.f, -1.f,  1.f},
                { 1.f,  1.f,  1.f},
                {-1.f,  1.f,  1.f},

                {-1.f, -1.f, -1.f},
                {-1.f, -1.f,  1.f},
                {-1.f,  1.f,  1.f},
                {-1.f, -1.f, -1.f},
                {-1.f,  1.f,  1.f},
                {-1.f,  1.f, -1.f},

                { 1.f,  1.f, -1.f},
                {-1.f,  1.f, -1.f},
                {-1.f,  1.f,  1.f},
                { 1.f,  1.f, -1.f},
                {-1.f,  1.f,  1.f},
                { 1.f,  1.f,  1.f},

                { 1.f, -1.f,  1.f},
                {-1.f, -1.f,  1.f},
                {-1.f, -1.f, -1.f},
                { 1.f, -1.f,  1.f},
                {-1.f, -1.f, -1.f},
                { 1.f, -1.f, -1.f}
            });

            using core::operator/;

            auto shader_dir = core::DEFAULT_CFG_PATH() / "../" /
                core::config_section::direct_read().read_unwrap<core::string>("shaders_dir");
            auto section = core::config_section::direct_read("shader_aabb_debug", core::cfg_reentry("debug_tools"));

            _program_id = _sm.compile_program(
                    shader_dir / section.read_unwrap<core::string>("effect_path"),
                    section.read_unwrap<core::string>("entry_function"));
            _mvp_uniform_id   = _sm.get_uniform_id_unwrap(_program_id, "MVP");
            _color_uniform_id = _sm.get_uniform_id_unwrap(_program_id, "aabb_color");
        }

    public:
        void draw(const grx_aabb& aabb, const glm::mat4& model, const glm::mat4& view_projection, const core::vec3f& color) const {
            auto scale = (aabb.max - aabb.min) / core::vec{2.f, 2.f, 2.f};
            auto pos   = (aabb.max + aabb.min) / core::vec{2.f, 2.f, 2.f};

            auto transform = glm::scale(glm::translate(glm::mat4(1.f), core::to_glm(pos)), core::to_glm(scale));

            //core::printline("pos: {.3}  scale: {.3}", pos, scale);

            _box.bind_vao();

            _sm.use_program(_program_id);
            _sm.set_uniform(_mvp_uniform_id, view_projection * (model * transform));
            _sm.set_uniform(_color_uniform_id, color);

            //grx_ctx().set_depth_test_enabled(false);
            grx_ctx().set_cull_face_enabled(false);
            grx_ctx().set_wireframe_enabled(true);
            _box.draw(36);
            grx_ctx().set_wireframe_enabled(false);
            grx_ctx().set_cull_face_enabled(true);
            //grx_ctx().set_depth_test_enabled(true);
        }

    private:
        grx_vbo_tuple<vbo_vector_vec3f> _box;
        grx_shader_mgr                  _sm;
        shader_program_id_t             _program_id;
        uniform_id_t                    _mvp_uniform_id;
        uniform_id_t                    _color_uniform_id;
    };

    inline grx_aabb_debug_drawer& grx_aabb_debug() {
        return grx_aabb_debug_drawer::instance();
    }
}

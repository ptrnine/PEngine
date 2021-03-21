#pragma once

#include "grx_vbo_tuple.hpp"

#include <core/config_manager.hpp>
#include <core/helper_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "grx_context.hpp"
#include "grx_shader.hpp"
#include "grx_types.hpp"
#include "grx_mesh_gen.hpp"

namespace grx
{

/**
 * @brief Singleton for drawing AABB's and collision shapes
 *
 * Use grx_aabb_debug to access the instance of this class
 *
 */
class grx_aabb_debug_drawer {
    SINGLETON_IMPL(grx_aabb_debug_drawer);

private:
    ~grx_aabb_debug_drawer() = default;
    grx_aabb_debug_drawer() {
        vbo_setup_as_box(_box);

        auto shader_vs = grx_shader<shader_type::vertex>(
            "in vec3 position_ms;",
            "layout(location = 1) in mat4 MVP;",
            "void main() { gl_Position = MVP * vec4(position_ms, 1.0); }");

        auto shader_fs = grx_shader<shader_type::fragment>(
            "uniform vec3 aabb_color;",
            "layout(location = 0) out vec4 color;",
            "layout(location = 1) out vec4 n;",
            "void main() { color = vec4(aabb_color, 1.0); n.w = 1.0; }");

        _program = grx_shader_program::create_shared(shader_vs, shader_fs);
        _color   = _program->get_uniform<core::vec3f>("aabb_color");

        auto ni_shader_vs = grx_shader<shader_type::vertex>(
            "uniform mat4 MVP;",
            "in vec3 position_ms;",
            "void main() { gl_Position = MVP * vec4(position_ms, 1.0); }");

        auto ni_shader_fs = grx_shader<shader_type::fragment>(
            "uniform vec3 aabb_color;",
            "layout(location = 0) out vec4 color;",
            "layout(location = 1) out vec4 n;",
            "void main() { color = vec4(aabb_color, 1.0); n.w = 1.0; }");

        _non_inst_program = grx_shader_program::create_shared(ni_shader_vs, ni_shader_fs);
        _non_inst_mvp     = _non_inst_program->get_uniform<glm::mat4>("MVP");
        _non_inst_color   = _non_inst_program->get_uniform<core::vec3f>("aabb_color");
    }

public:
    void enable(bool value = true) {
        _enable = value;
    }

    void disable() {
        enable(false);
    }

    bool is_enabled() const {
        return _enable;
    }

    void push(const grx_aabb&       aabb,
              const glm::mat4&      model,
              const grx::color_rgb& color) {
        if (!_enable || aabb.is_maximized())
            return;

        auto scale = (aabb.max - aabb.min) / core::vec{2.f, 2.f, 2.f}; // NOLINT
        auto pos   = (aabb.max + aabb.min) / core::vec{2.f, 2.f, 2.f}; // NOLINT
        auto transform =
            glm::scale(glm::translate(glm::mat4(1.f), core::to_glm(pos)), core::to_glm(scale));
        auto m = model * transform;
        _instances[color].push_back(m);
    }

    void draw_and_clear(const glm::mat4& view_projection) {
        if (!_enable)
            return;

        _box.bind_vao();
        _program->activate();

        grx_ctx().set_cull_face_enabled(false);
        grx_ctx().set_wireframe_enabled(true);

        for (auto& [color, models] : _instances) {
            _color = static_cast<vec3f>(color) * vec3f::filled_with(1/255.f);
            constexpr size_t rectangle_vertices_count = 36;

            for (auto& model : models)
                model = view_projection * model;

            _box.set_data<2>(models);
            _box.draw_instanced(models.size(), rectangle_vertices_count);
        }

        grx_ctx().set_wireframe_enabled(false);
        grx_ctx().set_cull_face_enabled(true);

        _instances.clear();
    }

    template <typename... Ts>
    void draw(grx_vbo_tuple<vbo_vector_indices, Ts...>& drawable,
              const glm::mat4&                          view_projection,
              const glm::mat4&                          model,
              const core::vec3f&                        color)
    {
        if (!_enable)
            return;

        drawable.bind_vao();

        _non_inst_program->activate();
        _non_inst_mvp   = view_projection * model;
        _non_inst_color = color;

        grx_ctx().set_cull_face_enabled(false);
        grx_ctx().set_wireframe_enabled(true);

        drawable.draw(drawable.indices_count().value());

        grx_ctx().set_wireframe_enabled(false);
        grx_ctx().set_cull_face_enabled(true);
    }

    struct color_comparator {
        bool operator()(const grx::color_rgb& lhs, const grx::color_rgb& rhs) const {
            core::u32 lhs_i = 0, rhs_i = 0; // NOLINT
            std::memcpy(&lhs_i, &lhs, sizeof(lhs));
            std::memcpy(&rhs_i, &rhs, sizeof(rhs));
            return lhs_i < rhs_i;
        }
    };

private:
    grx_vbo_tuple<vbo_vector_indices, vbo_vector_vec3f, vbo_vector_matrix4> _box;
    core::shared_ptr<grx_shader_program>                                    _program;
    core::map<grx::color_rgb, core::vector<glm::mat4>, color_comparator>    _instances;
    grx_uniform<core::vec3f>                                                _color;

    core::shared_ptr<grx_shader_program> _non_inst_program;
    grx_uniform<core::vec3f>             _non_inst_color;
    grx_uniform<glm::mat4>               _non_inst_mvp;
    bool                                 _enable = false;
};

/**
 * @brief Gets reference to grx_aabb_debug_drawer singleton instance
 *
 * @return the reference to grx_aabb_debug_drawer singleton instance
 */
inline grx_aabb_debug_drawer& grx_aabb_debug() {
    return grx_aabb_debug_drawer::instance();
}
} // namespace grx


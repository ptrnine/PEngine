#pragma once

#include "grx_vbo_tuple.hpp"

#include <core/config_manager.hpp>
#include <core/helper_macros.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "grx_context.hpp"
#include "grx_shader.hpp"
#include "grx_types.hpp"

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
        /* 2x2x2 rectangle */
        _box.set_data<0>({{1.f, -1.f, -1.f},  {-1.f, -1.f, -1.f}, {-1.f, 1.f, -1.f},
                          {1.f, -1.f, -1.f},  {-1.f, 1.f, -1.f},  {1.f, 1.f, -1.f},

                          {1.f, -1.f, 1.f},   {1.f, -1.f, -1.f},  {1.f, 1.f, -1.f},
                          {1.f, -1.f, 1.f},   {1.f, 1.f, -1.f},   {1.f, 1.f, 1.f},

                          {-1.f, -1.f, 1.f},  {1.f, -1.f, 1.f},   {1.f, 1.f, 1.f},
                          {-1.f, -1.f, 1.f},  {1.f, 1.f, 1.f},    {-1.f, 1.f, 1.f},

                          {-1.f, -1.f, -1.f}, {-1.f, -1.f, 1.f},  {-1.f, 1.f, 1.f},
                          {-1.f, -1.f, -1.f}, {-1.f, 1.f, 1.f},   {-1.f, 1.f, -1.f},

                          {1.f, 1.f, -1.f},   {-1.f, 1.f, -1.f},  {-1.f, 1.f, 1.f},
                          {1.f, 1.f, -1.f},   {-1.f, 1.f, 1.f},   {1.f, 1.f, 1.f},

                          {1.f, -1.f, 1.f},   {-1.f, -1.f, 1.f},  {-1.f, -1.f, -1.f},
                          {1.f, -1.f, 1.f},   {-1.f, -1.f, -1.f}, {1.f, -1.f, -1.f}});

        auto shader_vs = grx_shader<shader_type::vertex>(
            "uniform mat4 MVP;",
            "in vec3 position_ms;"
            "void main() { gl_Position = MVP * vec4(position_ms, 1.0); }");

        auto shader_fs = grx_shader<shader_type::fragment>(
            "uniform vec3 aabb_color;",
            "layout(location = 0) out vec4 color;",
            "layout(location = 1) out vec4 n;",
            "void main() { color = vec4(aabb_color, 1.0); n.w = 1.0; }");

        _program = grx_shader_program::create_shared(shader_vs, shader_fs);

        _mvp   = _program->get_uniform<glm::mat4>("MVP");
        _color = _program->get_uniform<core::vec3f>("aabb_color");
    }

public:
    /**
     * @brief Draws aabb
     *
     * @param aabb - the aabb
     * @param model - the model matrix
     * @param view_projection - (projection * view) matrix from camera
     * @param color - the color
     */
    void draw(const grx_aabb&    aabb,
              const glm::mat4&   model,
              const glm::mat4&   view_projection,
              const core::vec3f& color) const {
        auto scale = (aabb.max - aabb.min) / core::vec{2.f, 2.f, 2.f}; // NOLINT
        auto pos   = (aabb.max + aabb.min) / core::vec{2.f, 2.f, 2.f}; // NOLINT

        auto transform = glm::scale(glm::translate(glm::mat4(1.f), core::to_glm(pos)), core::to_glm(scale));

        _box.bind_vao();

        _program->activate();
        _mvp   = view_projection * (model * transform);
        _color = color;

        grx_ctx().set_cull_face_enabled(false);
        grx_ctx().set_wireframe_enabled(true);

        constexpr size_t rectangle_vertices_count = 36;
        _box.draw(rectangle_vertices_count);

        grx_ctx().set_wireframe_enabled(false);
        grx_ctx().set_cull_face_enabled(true);
    }

    template <typename... Ts>
    void draw(grx_vbo_tuple<vbo_vector_indices, Ts...>& drawable,
              const glm::mat4&                          view_projection,
              const glm::mat4&                          model,
              const core::vec3f&                        color)
    {
        drawable.bind_vao();

        _program->activate();
        _mvp = view_projection * model;
        _color = color;

        grx_ctx().set_cull_face_enabled(false);
        grx_ctx().set_wireframe_enabled(true);

        drawable.draw(drawable.indices_count().value());

        grx_ctx().set_wireframe_enabled(false);
        grx_ctx().set_cull_face_enabled(true);
    }

private:
    grx_vbo_tuple<vbo_vector_vec3f>      _box;
    core::shared_ptr<grx_shader_program> _program;
    grx_uniform<glm::mat4>               _mvp;
    grx_uniform<core::vec3f>             _color;
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


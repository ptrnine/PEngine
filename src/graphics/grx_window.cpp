#include "grx_window.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <core/assert.hpp>
#include <core/config_manager.hpp>
#include "grx_context.hpp"
#include "grx_shader_manager.hpp"

using namespace core;

static constexpr auto screen_quad_vertex_buffer = array {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f,  1.0f, 0.0f,
};


grx::grx_window::grx_window(
        const string& name,
        const vec2i& size,
        grx_shader_manager& shader_manager,
        config_manager& config_manager
) {
    grx::grx_context::instance();

    _wnd = glfwCreateWindow(size.x(), size.y(), name.data(), nullptr, nullptr);
    RASSERTF(_wnd, "{}", "Can't create GLFW window");

    auto savedContext = glfwGetCurrentContext();

    glfwMakeContextCurrent(_wnd);

    {
        glewExperimental = GL_TRUE; // required in core mode
        auto rc = glewInit();
        RASSERTF(rc == GLEW_OK, "Failed to initialize GLEW: {}", glewGetErrorString(rc));
    }

    // Input
    glfwSetInputMode(_wnd, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(_wnd, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

    // Render params
    glCullFace (GL_BACK);
    glEnable   (GL_CULL_FACE);
    glEnable   (GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Screen quad
    glGenBuffers(1, &_screen_quad_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, _screen_quad_vbo);
    glBufferData(
            GL_ARRAY_BUFFER,
            screen_quad_vertex_buffer.size() * sizeof(screen_quad_vertex_buffer[0]),
            screen_quad_vertex_buffer.data(),
            GL_STATIC_DRAW);

    _render_target = grx::grx_render_target(size);

    _screen_quad_passthrough = shader_manager.compile_program(config_manager, "shader_passthrough_screen_quad");
    shader_manager.get_uniform_id(_screen_quad_passthrough, "screen_quad_texture");

    // Restore context
    glfwMakeContextCurrent(savedContext);
}

grx::grx_window::~grx_window() {
    glDeleteBuffers(1, &_screen_quad_vbo);
}

void grx::grx_window::make_current() {
    glfwMakeContextCurrent(_wnd);
    grx::grx_ctx().bind_default_framebuffer();
}

bool grx::grx_window::should_close() {
    return glfwWindowShouldClose(_wnd);
}

void grx::grx_window::poll_events() {
    glfwPollEvents();
}

void grx::grx_window::swap_buffers() {
    glfwSwapBuffers(_wnd);
}

void grx::grx_window::bind_renderer() {
    _render_target.bind();
}

void grx::grx_window::present() {
    make_current();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _render_target.texture_id());

    grx::grx_shader_manager::use_program(_screen_quad_passthrough);
    grx::grx_shader_manager::set_uniform(_screen_quad_texture_uniform, 0);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, _screen_quad_vbo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);

    swap_buffers();
}
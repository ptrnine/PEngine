#include "grx_window.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <core/assert.hpp>
#include <core/config_manager.hpp>
#include "grx_context.hpp"
#include "grx_shader_mgr.hpp"
#include "grx_camera.hpp"

using namespace core;

namespace grx {
    static core::hash_map<GLFWwindow*, grx_window*> window_map;

    void grx_window::glfw_focus_callback(GLFWwindow* window, int focused) {
        window_map[window]->_on_focus = focused != 0;
    }
}

grx::grx_window::grx_window(
        const string& name,
        const vec2i& size,
        grx_shader_mgr& shader_manager,
        config_manager& config_manager
): _vbo_tuple(nullptr)
{
    grx::grx_context::instance();

    _wnd = glfwCreateWindow(size.x(), size.y(), name.data(), nullptr, nullptr);
    RASSERTF(_wnd, "{}", "Can't create GLFW window");

    _input_ctx.set_glfw_wnd(_wnd);
    _input_ctx.set_display_size(size);

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
    _vbo_tuple = grx_vbo_tuple<vbo_vector_vec3f>();
    _vbo_tuple.set_data<0>({
            {-1.0f, -1.0f, 0.0f },
            { 1.0f, -1.0f, 0.0f },
            {-1.0f,  1.0f, 0.0f },
            {-1.0f,  1.0f, 0.0f },
            { 1.0f, -1.0f, 0.0f },
            { 1.0f,  1.0f, 0.0f }
    });

    _render_target = grx_window_render_target(size);

    _screen_quad_passthrough = shader_manager.compile_program(config_manager, "shader_passthrough_screen_quad");
    _screen_quad_texture_uniform = shader_manager.get_uniform_id_unwrap(_screen_quad_passthrough, "screen_quad_texture");

    window_map.insert_or_assign(_wnd, this);
    glfwSetWindowFocusCallback(_wnd, grx_window::glfw_focus_callback);

    // Restore context
    glfwMakeContextCurrent(savedContext);
}

grx::grx_window::~grx_window() {
    window_map.erase(_wnd);
    glfwDestroyWindow(_wnd);
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

void grx::grx_window::bind_render_target() {
    _render_target.bind_render_target();
}

void grx::grx_window::bind_and_clear_render_target() {
    _render_target.bind_and_clear_render_target();
}

void grx::grx_window::present() {
    _render_target.do_postprocess_queue();

    make_current();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _vbo_tuple.bind_vao();

    _render_target.activate_texture();

    grx::grx_shader_mgr::use_program(_screen_quad_passthrough);
    grx::grx_shader_mgr::set_uniform(_screen_quad_texture_uniform, 0);

    //glEnable(GL_FRAMEBUFFER_SRGB);
    _vbo_tuple.draw(18);
    //glDisable(GL_FRAMEBUFFER_SRGB);

    swap_buffers();
}

core::vec2i grx::grx_window::size() const {
    int x, y;
    glfwGetWindowSize(_wnd, &x, &y);
    return core::vec{x, y};
}

void grx::grx_window::set_mouse_pos(const core::vec2f& position) {
    glfwSetCursorPos(_wnd, position.x(), position.y());
}

void grx::grx_window::reset_mouse_pos() {
    auto size = this->size();
    set_mouse_pos(static_cast<vec2f>(size) / 2.f);
}

void grx::grx_window::update_input()  {
    _input_ctx.update();
    _camera->update(this);
}
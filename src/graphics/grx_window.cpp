#include "grx_window.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <core/assert.hpp>
#include <core/config_manager.hpp>
#include <core/math.hpp>
#include "grx_context.hpp"
#include "grx_shader_mgr.hpp"
#include "grx_camera.hpp"

using namespace core;

namespace grx {
    static GLFWwindow* main_window_context;
    static core::hash_map<GLFWwindow*, grx_window*> window_map;
}

grx::grx_window::grx_window(
        const string& name,
        const vec2i& size,
        grx_shader_mgr& shader_manager,
        config_manager& config_manager,
        const core::shared_ptr<grx_window_render_target>& render_target
) {
    grx::grx_context::instance();

    glfwWindowHint( GLFW_DOUBLEBUFFER, GL_FALSE );
    _wnd = glfwCreateWindow(size.x(), size.y(), name.data(), nullptr,
            main_window_context ? main_window_context : nullptr);
    RASSERTF(_wnd, "{}", "Can't create GLFW window");

    if (!main_window_context)
        main_window_context = _wnd;

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

    _render_target = render_target ?
        render_target :
        grx_window_render_target::create_shared(size);

    _screen_quad_passthrough = shader_manager.compile_program(config_manager, "shader_passthrough_screen_quad");
    _screen_quad_texture_uniform = shader_manager.get_uniform_id_unwrap(_screen_quad_passthrough, "screen_quad_texture");

    window_map.insert_or_assign(_wnd, this);

    _input_mgr = inp::inp_ctx().input_mgr_for(_wnd);
    _input_mgr->SetDisplaySize(size.x(), size.y());
    _mouse_id    = _input_mgr->CreateDevice<gainput::InputDeviceMouse>();
    _keyboard_id = _input_mgr->CreateDevice<gainput::InputDeviceKeyboard>();

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
    _render_target->bind_render_target();
}

void grx::grx_window::bind_and_clear_render_target() {
    _render_target->bind_and_clear_render_target();
}

void grx::grx_window::present() {
    _render_target->do_postprocess_queue();

    make_current();
    glClearColor(119.f / 255.f, 162.f / 255.f, 155.f / 255.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    _render_target->bind_quad_vao();

    _render_target->activate_texture();

    grx::grx_shader_mgr::use_program(_screen_quad_passthrough);
    grx::grx_shader_mgr::set_uniform(_screen_quad_texture_uniform, 0);

    //glEnable(GL_FRAMEBUFFER_SRGB);
    _render_target->draw_quad();
    //_vbo_tuple.draw(18);
    //glDisable(GL_FRAMEBUFFER_SRGB);

    swap_buffers();
}

core::vec2i grx::grx_window::size() const {
    int x, y;
    glfwGetWindowSize(_wnd, &x, &y);
    return core::vec{x, y};
}

void grx::grx_window::set_mouse_pos(const core::vec2f& position) {
    auto sz  = static_cast<core::vec2f>(size());
    auto pos = static_cast<core::vec2i>(core::vec2f{
        roundf(core::lerp(0.f, sz.x(), position.x())),
        roundf(core::lerp(0.f, sz.y(), position.y()))});
    inp::inp_ctx().window_state_for(_wnd).mouse_pos.set_and_activate(pos);
}

void grx::grx_window::set_pos(const core::vec2i& position) {
    glfwSetWindowPos(_wnd, position.x(), position.y());
}

void grx::grx_window::reset_mouse_pos() {
    set_mouse_pos({0.5f, 0.5f});
}

void grx::grx_window::update_input() {
    //if (on_focus())
        _input_mgr->Update();

    if (_camera)
        _camera->update(this);
}

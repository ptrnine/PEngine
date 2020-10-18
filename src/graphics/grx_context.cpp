#include "grx_context.hpp"
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <core/assert.hpp>
#include <core/log.hpp>

static_assert(std::is_same_v<float, GLfloat>);
static_assert(std::is_same_v<uint, GLuint>);

static void glfw_error(int id, const char *description) {
    std::cerr << description << std::endl;
    exit(id);
}

const char* gl_severity_str(GLenum severity) {
    switch (severity) {
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return "notification";
    case GL_DEBUG_SEVERITY_LOW:
        return "medium";
    case GL_DEBUG_SEVERITY_MEDIUM:
        return "medium";
    case GL_DEBUG_SEVERITY_HIGH:
        return "high";
    default:
        return "*unknown*";
    }
}

const char* gl_debug_type_str(GLenum type) {
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        return "error";
    case GL_DEBUG_TYPE_PERFORMANCE:
        return "performance";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        return "deprecated behavior";
    case GL_DEBUG_TYPE_MARKER:
        return "marker";
    case GL_DEBUG_TYPE_POP_GROUP:
        return "pop group";
    case GL_DEBUG_TYPE_PORTABILITY:
        return "portability";
    case GL_DEBUG_TYPE_PUSH_GROUP:
        return "push group";
    case GL_DEBUG_TYPE_OTHER:
        return "other";
    default:
        return "*undefined*";
    }
}

const char* gl_debug_source_str(GLenum source) {
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        return "OpenGL API";
    case GL_DEBUG_SOURCE_APPLICATION:
        return "application";
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        return "GLSL compiler";
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        return "window system";
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        return "thirt party library or debuger";
    case GL_DEBUG_SOURCE_OTHER:
        return "other";
    default:
        return "*undefined*";
    }
}

void GLAPIENTRY gl_message_callback(
        GLenum        source,
        GLenum        type,
        GLuint      /*id*/,
        GLenum        severity,
        GLsizei     /*length*/,
        const GLchar* message,
        const GLvoid* /*user_param*/
) {
    core::logger::Type log_type = core::logger::Message;

    if (severity == GL_DEBUG_SEVERITY_MEDIUM)
        log_type = core::logger::Warning;
    else if (severity == GL_DEBUG_SEVERITY_HIGH)
        log_type = core::logger::Error;

    /* TODO: add option for enable 'notification' with 'other' */
    if (severity != GL_DEBUG_SEVERITY_NOTIFICATION || type != GL_DEBUG_TYPE_OTHER)
        core::log().write(log_type,
            "{} [{}, {}]: {}",
            gl_debug_source_str(source),
            gl_severity_str    (severity),
            gl_debug_type_str  (type),
            message);
}

grx::grx_context::grx_context() {
    glfwSetErrorCallback(&glfw_error);
    RASSERTF(glfwInit(), "{}", "Failed to initialize glfw");

    // Antialiasing
    //glfwWindowHint(GLFW_SAMPLES, 4);
    glDisable(GL_MULTISAMPLE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
}

grx::grx_context::~grx_context() {
    glfwTerminate();
}

void grx::grx_context::setup_debug_callback() {
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(gl_message_callback, 0);
}

void grx::grx_context::bind_default_framebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void grx::grx_context::set_wireframe_enabled(bool value) {
    if (value) {
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE);
        _is_wireframe_enabled = true;
    } else {
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
        _is_wireframe_enabled = false;
    }
}

void grx::grx_context::set_depth_test_enabled(bool value) {
    if (value) {
        glEnable(GL_DEPTH_TEST);
        _is_depth_test_enabled = true;
    } else {
        glDisable(GL_DEPTH_TEST);
        _is_depth_test_enabled = false;
    }
}

void grx::grx_context::set_depth_mask_enabled(bool value) {
    if (value) {
        glDepthMask(GL_TRUE);
        _is_depth_mask_enabled = true;
    } else {
        glDepthMask(GL_FALSE);
        _is_depth_mask_enabled = false;
    }
}

void grx::grx_context::set_cull_face_enabled(bool value) {
    if (value)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    _is_cull_face_enabled = value;
}

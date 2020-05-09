#include "grx_context.hpp"
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <core/assert.hpp>

static_assert(std::is_same_v<float, GLfloat>);
static_assert(std::is_same_v<uint, GLuint>);

static void glfw_error(int id, const char *description) {
    std::cerr << description << std::endl;
    exit(id);
}

grx::grx_context::grx_context() {
    glfwSetErrorCallback(&glfw_error);
    RASSERTF(glfwInit(), "{}", "Failed to initialize glfw");

    // Antialiasing
    glfwWindowHint(GLFW_SAMPLES, 4);
    glEnable(GL_MULTISAMPLE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
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
        _is_wireframe_enabled = false;
    }
}

void grx::grx_context::set_cull_face_enabled(bool value) {
    if (value)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);

    _is_cull_face_enabled = value;
}

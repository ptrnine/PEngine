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

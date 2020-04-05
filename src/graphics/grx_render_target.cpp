#include "grx_render_target.hpp"
#include <core/assert.hpp>

#include "grx_context.hpp"

#include <GL/glew.h>
#include <GL/glfx.h>
#include <GLFW/glfw3.h>

grx::grx_render_target::grx_render_target(const core::vec2i& size) : _size(size) {
    grx::grx_context::instance();

    glGenFramebuffers(1, &_framebuffer_id);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer_id);

    glGenTextures(1, &_texture_id);
    glBindTexture(GL_TEXTURE_2D, _texture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x(), size.y(), 0,GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenRenderbuffers(1, &_depthbuffer_id);
    glBindRenderbuffer(GL_RENDERBUFFER, _depthbuffer_id);

    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.x(), size.y());
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _depthbuffer_id);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, _texture_id, 0);
    GLenum draw_buffers[1] = {GL_COLOR_ATTACHMENT0};
    glDrawBuffers(GL_COLOR_ATTACHMENT0, draw_buffers);

    RASSERTF(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
            "{}", "Error while creating render target");
}

grx::grx_render_target::~grx_render_target() {
    if (_framebuffer_id != std::numeric_limits<uint>::max())
        glDeleteFramebuffers(1, &_framebuffer_id);

    if (_depthbuffer_id != std::numeric_limits<uint>::max())
        glDeleteRenderbuffers(1, &_depthbuffer_id);

    if (_texture_id != std::numeric_limits<uint>::max())
        glDeleteTextures(1, &_texture_id);
}

void grx::grx_render_target::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer_id);
    glViewport(0,0, _size.x(), _size.y());
}
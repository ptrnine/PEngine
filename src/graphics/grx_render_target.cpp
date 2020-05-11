#include "grx_render_target.hpp"
#include <core/assert.hpp>

#include "grx_context.hpp"

#include <GL/glew.h>
#include <GL/glfx.h>
#include <GLFW/glfw3.h>

inline GLenum color_fmt_to_opengl(grx::grx_color_fmt fmt) {
    core::array map = {
            GL_RGB, GL_SRGB, GL_RGB16, GL_RGB16F, GL_RGB32F
    };
    return static_cast<uint>(map[static_cast<uint>(fmt)]);
}

inline GLint filtering_to_opengl(grx::grx_filtering filtering) {
    static core::array map = {
            GL_LINEAR, GL_NEAREST
    };
    return map[static_cast<uint>(filtering)];
}

void grx::_grx_render_target_tuple_init(
        core::pair<grx_color_fmt, grx_filtering>* settings,
        const core::vec2i& size,
        uint* framebuffer_ids,
        uint* texture_ids,
        uint* depthbuffer_ids,
        size_t count
) {
    grx_ctx();

    glGenFramebuffers (static_cast<GLsizei>(count), framebuffer_ids);
    glGenTextures     (static_cast<GLsizei>(count), texture_ids);
    glGenRenderbuffers(static_cast<GLsizei>(count), depthbuffer_ids);

    for (size_t i = 0; i < count; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_ids[i]);
        glBindTexture(GL_TEXTURE_2D, texture_ids[i]);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, size.x(), size.y(), 0, color_fmt_to_opengl(settings[i].first),
                     GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering_to_opengl(settings[i].second));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering_to_opengl(settings[i].second));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer_ids[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, size.x(), size.y());
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer_ids[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_ids[i], 0);
        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, draw_buffers);

        RASSERTF(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                 "{}", "Error while creating render target");
    }
}

void grx::_grx_render_target_tuple_delete(
        uint* framebuffer_ids,
        uint* texture_ids,
        uint* depthbuffer_ids,
        size_t count
) {
    glDeleteFramebuffers (static_cast<GLsizei>(count), framebuffer_ids);
    glDeleteRenderbuffers(static_cast<GLsizei>(count), depthbuffer_ids);
    glDeleteTextures     (static_cast<GLsizei>(count), texture_ids);
}

void grx::_grx_render_target_tuple_bind(uint framebuffer_id, const core::vec2i& size) {
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_id);
    glViewport(0,0, size.x(), size.y());
}

void grx::_grx_render_target_tuple_active_texture(uint texture_id) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
}

void grx::_grx_render_target_tuple_clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

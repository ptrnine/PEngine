#include "grx_render_target.hpp"
#include <core/assert.hpp>

#include "grx_context.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

inline auto color_fmt_to_opengl(grx::grx_color_fmt fmt) {
    core::array map = {
            GL_RGB, GL_SRGB, GL_RGB16, GL_RGB16F, GL_RGB32F
    };
    return map[static_cast<uint>(fmt)]; // NOLINT
}

inline GLint filtering_to_opengl(grx::grx_filtering filtering) {
    static core::array map = {
            GL_LINEAR, GL_NEAREST
    };
    return map[static_cast<uint>(filtering)]; // NOLINT
}

void grx::_grx_render_target_tuple_init(
        core::tuple<grx_color_fmt, grx_filtering, bool>* settings,
        const core::vec2u& size,
        uint* framebuffer_ids,
        uint* texture_ids,
        uint* depthbuffer_ids,
        size_t count
) {
    auto [color_fmt, filtering, enable_mipmaps] = *settings;

    grx_ctx();

    glGenFramebuffers (static_cast<GLsizei>(count), framebuffer_ids);
    glGenTextures     (static_cast<GLsizei>(count), texture_ids);
    glGenRenderbuffers(static_cast<GLsizei>(count), depthbuffer_ids);

    for (size_t i = 0; i < count; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_ids[i]);
        glBindTexture(GL_TEXTURE_2D, texture_ids[i]);

        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     color_fmt_to_opengl(color_fmt),
                     static_cast<GLsizei>(size.x()),
                     static_cast<GLsizei>(size.y()),
                     0,
                     GL_RGB,
                     GL_FLOAT,
                     nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering_to_opengl(filtering));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering_to_opengl(filtering));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if (enable_mipmaps)
            glGenerateMipmap(GL_TEXTURE_2D);

        glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer_ids[i]);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                static_cast<GLsizei>(size.x()), static_cast<GLsizei>(size.y()));
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthbuffer_ids[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_ids[i], 0);
        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 }; // NOLINT
        glDrawBuffers(1, draw_buffers);

        PeRelRequireF(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
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

void grx::_grx_render_target_tuple_bind(uint framebuffer_id, const core::vec2u& size) {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer_id);
    glViewport(0,0, static_cast<GLsizei>(size.x()), static_cast<GLsizei>(size.y()));
}

void grx::_grx_render_target_tuple_active_texture(uint texture_id) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
}

void grx::_grx_render_target_tuple_clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void grx::_grx_render_target_tuple_generate_mipmap(uint texture_id) {
    glGenerateTextureMipmap(texture_id);
}

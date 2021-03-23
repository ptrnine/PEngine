#include "grx_g_buffer.hpp"

#include <GL/glew.h>

namespace {
void gen_vec4_attachment(uint& id, core::vec2i size, GLenum attachment) {
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, size.x(), size.y(), 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, id, 0);
}

/*
void gen_color_attachment(uint& id, core::vec2i size, GLenum attachment) {
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, size.x(), size.y(), 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, id, 0);
}
*/
}

uint grx::details::gen_g_buffer(vec2i size, uint* attachments) {
    uint fbo; // NOLINT
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    gen_vec4_attachment(attachments[grx_g_buffer::gbuf_albedo],         size, GL_COLOR_ATTACHMENT0);
    gen_vec4_attachment(attachments[grx_g_buffer::gbuf_normal],         size, GL_COLOR_ATTACHMENT1);
    gen_vec4_attachment(attachments[grx_g_buffer::gbuf_position_depth], size, GL_COLOR_ATTACHMENT2);
    gen_vec4_attachment(attachments[grx_g_buffer::gbuf_final],          size, GL_COLOR_ATTACHMENT3);

    glGenTextures(1, &attachments[grx_g_buffer::gbuf_depth]);
    glBindTexture(GL_TEXTURE_2D, attachments[grx_g_buffer::gbuf_depth]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, size.x(), size.y(),
            0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
            attachments[grx_g_buffer::gbuf_depth], 0);

    std::array<uint, 3> attach = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
    };
    glDrawBuffers(static_cast<GLsizei>(attach.size()), attach.data());

    auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Framebuffer error: " + std::to_string(status));

    return fbo;
}

void grx::details::bind_g_buffer(uint fbo) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void grx::details::bind_and_clear_g_buffer(uint fbo) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void grx::details::activate_g_buffer_textures(uint *attachments) {
#define BIND_UNIT(GBUF_ATTACHMENT) \
    glBindTextureUnit(GBUF_ATTACHMENT, attachments[GBUF_ATTACHMENT])

    BIND_UNIT(grx_g_buffer::gbuf_albedo);
    BIND_UNIT(grx_g_buffer::gbuf_normal);
    BIND_UNIT(grx_g_buffer::gbuf_position_depth);

#undef BIND_UNIT
}

void grx::grx_g_buffer::bind_geometry_pass() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    std::array<uint, 3> attach = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
    };
    glDrawBuffers(static_cast<GLsizei>(attach.size()), attach.data());

    /*
    glDepthMask(GL_TRUE);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    std::array<uint, 3> attach = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2
    };
    glDrawBuffers(static_cast<GLsizei>(attach.size()), attach.data());

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    */
}

void grx::grx_g_buffer::bind_stencil_path() {
    glDrawBuffer(GL_NONE);
    /*
    glDepthMask(GL_FALSE);

    glDrawBuffer(GL_NONE);

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glClear(GL_STENCIL_BUFFER_BIT);

    glStencilFunc(GL_ALWAYS, 0, 0);
    glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR, GL_KEEP);
    glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR, GL_KEEP);
    */
    /* Render sphere here */
}


void grx::grx_g_buffer::clear_result_attachment() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    glClear(GL_COLOR_BUFFER_BIT);

}

void grx::grx_g_buffer::bind_lighting_pass() {
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    activate_textures();
    /*
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
    glDrawBuffer(GL_COLOR_ATTACHMENT3);
    glClear(GL_COLOR_BUFFER_BIT);

    activate_textures();

    glDepthMask(GL_FALSE);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);


    glClear(GL_COLOR_BUFFER_BIT);
    */
}

void grx::grx_g_buffer::bind_present_pass() {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glReadBuffer(GL_COLOR_ATTACHMENT3);
}

/*
void grx::grx_g_buffer::end_lighting_pass() {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}
*/


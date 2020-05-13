#include "grx_texture.hpp"

#include <core/assert.hpp>

#include <GL/glew.h>
#include <GL/glfx.h>

#include "grx_gl_trace.hpp"


namespace grx::grx_texture_helper {
    /*
    class fbo_helper {
        SINGLETON_IMPL(fbo_helper);

    public:
        fbo_helper() {
            glGenVertexArrays(1, &_vao);
            glBindVertexArray(_vao);
            glGenBuffers(1, &_quad_vbo);

            auto vertices = core::array{
                -1.f, -1.f, 0.f,
                 1.f, -1.f, 0.f,
                -1.f,  1.f, 0.f,
                -1.f,  1.f, 0.f,
                 1.f, -1.f, 0.f,
                 1.f,  1.f, 0.f
            };

            glBindBuffer(GL_ARRAY_BUFFER, _quad_vbo);
            glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), vertices.data(), GL_STATIC_DRAW);
            //glEnableVertexAttribArray(0);
            //glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

            glGenFramebuffers(1, &_fbo_id);
            glGenRenderbuffers(1, &_rbo_id);

            _effect_id = glfxGenEffect();
            auto rc = glfxParseEffectFromMemory(_effect_id,
                "uniform sampler2D texture0;\n"
                "shader quad_vs(in vec3 pos, out vec2 uv) {\n"
                "    gl_Position = vec4(pos, 1.0);\n"
                "    uv = (pos.xy + vec2(1.0, 1.0)) * 0.5;\n"
                "}\n"
                "shader quad_fs(in vec2 uv, out vec3 color) {\n"
                "    color = texture(texture0, uv).rgb;\n"
                "}\n"
                "program quad {\n"
                "    vs(410) = quad_vs();\n"
                "    fs(410) = quad_fs();\n"
                "};\n"
            );

            RASSERTF(rc, "Error while parsing effect: {}", glfxGetEffectLog(_effect_id));

            auto program_int_id = glfxCompileProgram(_effect_id, "quad");

            RASSERTF(program_int_id >= 0, "Error while compile program: {}", glfxGetEffectLog(_effect_id));
            _program_id = static_cast<uint>(program_int_id);

            _texture_uniform = glGetUniformLocation(_program_id, "texture0");

            //glBindFramebuffer(GL_FRAMEBUFFER, _gl_framebuffer_id);
            //RASSERTF(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "{}", "fuck you");
        }

        ~fbo_helper() {
            glDeleteProgram     (_program_id);
            glfxDeleteEffect    (_effect_id);
            glDeleteRenderbuffers(1, &_rbo_id);
            glDeleteFramebuffers(1, &_fbo_id);
            glDeleteBuffers     (1, &_quad_vbo);
            glDeleteVertexArrays(1, &_vao);
        }

        uint make_texture_copy(uint src_id, uint channels_count, uint w, uint h, bool is_float) {
        }

    private:
        uint _fbo_id;
        uint _rbo_id;
        uint _vao;
        uint _quad_vbo;
        int  _effect_id;
        uint _program_id;
        int  _texture_uniform;
    };
    */

    inline GLenum format_from_channels(uint channels_count) {
        switch (channels_count) {
        case 1:
            return GL_RED;
        case 2:
            return GL_RG;
        case 3:
            return GL_RGB;
        case 4:
            return GL_RGBA;
        default:
            ABORTF("Wrong color channels count {}", channels_count);
            return 0;
        }
    }

    uint generate_gl_texture() {
        GLuint id;
        GL_TRACE(glGenTextures, 1, &id);
        return id;
    }

    void delete_gl_texture(uint id) {
        if (id != grx_texture<1>::no_id)
            GL_TRACE(glDeleteTextures, 1, &id);
    }

    void copy_texture(uint dst_id, uint src_id, uint w, uint h) {
        LOG("GL Texture copy: {} to {}", src_id, dst_id);
        GL_TRACE(glCopyImageSubData,
                src_id, GL_TEXTURE_2D, 0, 0, 0, 0,
                dst_id, GL_TEXTURE_2D, 0, 0, 0, 0,
                static_cast<GLsizei>(w),
                static_cast<GLsizei>(h),
                1);
        GL_TRACE(glBindTexture, GL_TEXTURE_2D, dst_id);
        GL_TRACE(glGenerateMipmap, GL_TEXTURE_2D);
    }

    void reset_texture(uint id, uint channels_count, uint w, uint h, bool is_float, const void* color_map_data, bool gen_mipmap) {
        gl_bind_texture(id);

        GLenum format = format_from_channels(channels_count);
        GLenum type   = is_float ? GL_FLOAT : GL_UNSIGNED_BYTE;

        GL_TRACE(glTexSubImage2D,
                GL_TEXTURE_2D,
                0,
                0,
                0,
                static_cast<GLsizei>(w),
                static_cast<GLsizei>(h),
                format,
                type,
                color_map_data);

        if (gen_mipmap)
            GL_TRACE(glGenerateMipmap, GL_TEXTURE_2D);
    }

    void setup_texture(uint id, uint channels_count, uint w, uint h, bool is_float, const void* color_map_data, bool gen_mipmap) {
        gl_bind_texture(id);

        GLenum format = format_from_channels(channels_count);
        GLenum type   = is_float ? GL_FLOAT : GL_UNSIGNED_BYTE;

        GL_TRACE(glTexImage2D,
                GL_TEXTURE_2D,
                0,
                static_cast<GLint>(format),
                static_cast<GLsizei>(w),
                static_cast<GLsizei>(h),
                0,
                format,
                type,
                color_map_data);

        GL_TRACE(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_TRACE(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

        if (gen_mipmap)
            GL_TRACE(glGenerateMipmap, GL_TEXTURE_2D);
    }

    void gl_active_texture(uint num) {
        GL_TRACE(glActiveTexture, GL_TEXTURE0 + num);
    }

    void gl_bind_texture(uint id) {
        GL_TRACE(glBindTexture, GL_TEXTURE_2D, id);
    }

    void get_texture(void* dst, uint channels_count, bool is_float) {
        GLenum format = format_from_channels(channels_count);
        GLenum type   = is_float ? GL_FLOAT : GL_UNSIGNED_BYTE;
        GL_TRACE(glGetTexImage, GL_TEXTURE_2D, 0, format, type, dst);
    }
}


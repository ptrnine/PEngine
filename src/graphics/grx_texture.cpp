#include "grx_texture.hpp"

#include <core/assert.hpp>

#include <GL/glew.h>

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

    inline GLenum internal_format_from_channels(uint channels_count) {
        switch (channels_count) {
        case 1:
            return GL_R8;
        case 2:
            return GL_RG8;
        case 3:
            return GL_RGB8;
        case 4:
            return GL_RGBA8;
        default:
            ABORTF("Wrong color channels count {}", channels_count);
            return 0;
        }
    }

    uint create_texture() {
        uint name; // NOLINT
        GL_TRACE(glCreateTextures, GL_TEXTURE_2D, 1, &name);
        return name;
    }

    void delete_texture(uint name) {
        if (name != grx_texture<1>::no_name)
            GL_TRACE(glDeleteTextures, 1, &name);
    }

    void generate_storage(uint name, uint w, uint h, uint channels) {
        auto internal_fmt = internal_format_from_channels(channels);

        GL_TRACE(glTextureStorage2D,
                name,
                10,
                internal_fmt,
                static_cast<GLsizei>(w),
                static_cast<GLsizei>(h));
    }

    uint create_texture(uint w, uint h, uint channels) {
        auto name = create_texture();
        generate_storage(name, w, h, channels);
        return name;
    }

    void set_storage(uint name, uint w, uint h, uint channels, bool is_float, const void* color_map_data) {
        GLenum format = format_from_channels(channels);
        GLenum type   = is_float ? GL_FLOAT : GL_UNSIGNED_BYTE;

        GL_TRACE(glTextureSubImage2D,
                name,
                0,
                0,
                0,
                static_cast<GLsizei>(w),
                static_cast<GLsizei>(h),
                format,
                type,
                color_map_data);

        GL_TRACE(glGenerateTextureMipmap, name);

        //GL_TRACE(glTextureParameteri, name, GL_TEXTURE_WRAP_S, GL_REPEAT);
        //GL_TRACE(glTextureParameteri, name, GL_TEXTURE_WRAP_T, GL_REPEAT);
        GL_TRACE(glTextureParameteri, name, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_TRACE(glTextureParameteri, name, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }

    uint create_texture(uint w, uint h, uint channels, bool is_float, const void* color_map_data) {
        auto name = create_texture(w, h, channels);
        set_storage(name, w, h, channels, is_float, color_map_data);
        return name;
    }

    void copy_texture(uint dst_name, uint src_name, uint w, uint h) {
        GL_TRACE(glCopyImageSubData,
                src_name, GL_TEXTURE_2D, 0, 0, 0, 0,
                dst_name, GL_TEXTURE_2D, 0, 0, 0, 0,
                static_cast<GLsizei>(w),
                static_cast<GLsizei>(h),
                1);

        GL_TRACE(glGenerateTextureMipmap, dst_name);

        GL_TRACE(glTextureParameteri, dst_name, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        GL_TRACE(glTextureParameteri, dst_name, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    }

    void get_texture(void* dst, uint src_name, uint x, uint y, uint channels, bool is_float) {
        GLenum format = format_from_channels(channels);
        GLenum type   = is_float ? GL_FLOAT : GL_UNSIGNED_BYTE;
        GL_TRACE(glGetTextureImage,
                src_name, 0, format, type, static_cast<GLsizei>(x * y * channels * (is_float ? sizeof(float) : 1)), dst);
    }

    void bind_unit(uint name, uint number) {
        GL_TRACE(glBindTextureUnit, number, name);
    }

    void active_texture(uint number) {
        GL_TRACE(glActiveTexture, GL_TEXTURE0 + number);
    }

    void bind_texture(uint name) {
        GL_TRACE(glBindTexture, GL_TEXTURE_2D, name);
    }

/*
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

        auto internal_fmt = internal_format_from_channels(channels_count);
        GLenum format = format_from_channels(channels_count);
        GLenum type   = is_float ? GL_FLOAT : GL_UNSIGNED_BYTE;

        GL_TRACE(glTexStorage2D,
                GL_TEXTURE_2D,
                1,
                internal_fmt,
                static_cast<GLsizei>(w),
                static_cast<GLsizei>(h));
        //glTexStorage2D(GL_TEXTURE_2D, 0, );
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

        //GL_TRACE(glTexImage2D,
        //        GL_TEXTURE_2D,
        //        0,
        //        static_cast<GLint>(format),
        //        static_cast<GLsizei>(w),
        //        static_cast<GLsizei>(h),
        //        0,
        //        format,
        //        type,
        //        color_map_data);

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
    */
}


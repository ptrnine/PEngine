#include "grx_texture.hpp"

#include <core/assert.hpp>

#include <GL/glew.h>

#include "grx_gl_trace.hpp"


namespace grx::grx_texture_helper {
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

    inline GLenum gl_texture_access(texture_access access) {
        switch (access) {
        case texture_access::read:      return GL_READ_ONLY;
        case texture_access::write:     return GL_WRITE_ONLY;
        case texture_access::readwrite: return GL_READ_WRITE;
        default:
            ABORTF("{}", "Unknown access specifier");
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

    void bind_image_texture(uint unit, uint name, int level, texture_access access, uint channels) {
        GL_TRACE(glBindImageTexture,
                unit,
                name,
                level,
                GL_FALSE,
                0,
                gl_texture_access(access),
                internal_format_from_channels(channels));
    }
}


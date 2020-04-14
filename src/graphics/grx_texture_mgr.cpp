#include "grx_texture_mgr.hpp"
#include <GL/glew.h>

#include <core/container_extensions.hpp>
#include <core/log.hpp>
#include <core/assert.hpp>

extern "C" {
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
}

using core::operator/;

auto grx::grx_texture_mgr::load(core::string_view p) -> grx_texture {
    auto path = core::path_eval(p);

    auto [position, was_inserted] = _texture_ids.emplace(path, grx_texture());

    if (!was_inserted) {
        return position->second;
    } else {
        int w, h, comp;
        auto img = stbi_load(path.data(), &w, &h, &comp, STBI_rgb_alpha);

        if (!img) {
            LOG_WARNING("Can't load texture at path '{}'", path);
            RABORTF("Can't load texture at path'{}'", path);
            return grx_texture(); // Todo: dummy texture
        }
        else {
            GLuint texture_id;
            glGenTextures(1, &texture_id);
            glBindTexture(GL_TEXTURE_2D, texture_id);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

            stbi_image_free(img);

            return position->second = {
                static_cast<uint>(w),
                static_cast<uint>(h),
                static_cast<uint>(comp),
                static_cast<texture_id_t>(texture_id)
            };
        }
    }
}

auto grx::grx_texture_mgr::load(const core::config_manager& cm, core::string_view p) -> grx_texture {
    return load(cm.entry_dir() / cm.read_unwrap<core::string>("textures_dir") / p);
}

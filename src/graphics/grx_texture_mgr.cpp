#include "grx_texture_mgr.hpp"

#include <core/log.hpp>
#include <core/config_manager.hpp>

using namespace core;

grx::grx_texture_mgr::grx_texture_mgr(const config_manager& cm) {
    _textures_dir = path_eval(cm.entry_dir() / cm.read_unwrap<string>("textures_dir"));
}

namespace {
    template <size_t S>
    using texture_map_t = hash_map<grx::grx_texture_mgr::path_t, grx::grx_texture_mgr::texture_val_t<S>>;

    template <size_t S>
    inline void unload_texture(texture_map_t<S>& textures, const string& path, bool load_to_map) {
        auto pos = textures.find(path);
        if (pos != textures.end()) {
            auto& tex = pos->second;

            if (tex.texture) {
                if (load_to_map)
                    tex.image = tex.texture->to_color_map();
                tex.texture.reset();
            } else {
                LOG_WARNING("Can't unload texture '{}': texture not loaded!");
            }

        } else {
            LOG_WARNING("Can't unload texture '{}': not found in manager!", path);
        }
    }

    template <size_t S, typename T>
    inline void try_unload(T& val, texture_map_t<S>& textures, const string& path) {
        if (val.usages == 0 && val.load_significance != grx::texture_load_significance::high)
            unload_texture(textures, path, val.load_significance == grx::texture_load_significance::medium);
    }
}


/*
auto grx::grx_texture_mgr::load(core::string_view p) -> core::optional<grx_texture> {
    auto path = core::path_eval(p);

    std::lock_guard lock(_texture_ids_mutex);

    auto [position, was_inserted] = _texture_ids.emplace(path, grx_texture());

    if (!was_inserted) {
        return position->second;
    } else {
        int w, h, comp;
        auto img = stbi_load(path.data(), &w, &h, &comp, STBI_rgb_alpha);

        if (!img) {
            return core::nullopt;
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

            return position->second = grx_texture{
                static_cast<uint>(w),
                static_cast<uint>(h),
                static_cast<uint>(comp),
                static_cast<texture_id_t>(texture_id)
            };
        }
    }
}

auto grx::grx_texture_mgr::load(const core::config_manager& cm, core::string_view p) -> core::optional<grx_texture> {
    return load(cm.entry_dir() / cm.read_unwrap<core::string>("textures_dir") / p);
}

auto grx::grx_texture_mgr::load_unwrap(core::string_view path) -> grx_texture {
    auto texture = load(path);
    PeRelRequireF(texture.has_value(), "Can't load texture at path'{}'", path);
    return *texture;
}

auto grx::grx_texture_mgr::load_unwrap(const core::config_manager& cm, core::string_view p) -> grx_texture {
    auto texture = load(cm, p);
    PeRelRequireF(texture.has_value(), "Can't load texture at path'{}'",
            core::path_eval(cm.entry_dir() / cm.read_unwrap<core::string>("textures_dir") / p));
    return *texture;
}

namespace grx {
    constexpr std::array textures_uniform_names = {
            "texture0",  "texture1",  "texture2",  "texture3",  "texture4",  "texture5",  "texture6",  "texture7",
            "texture8",  "texture9",  "texture10", "texture11", "texture12", "texture13", "texture14", "texture15",
            "texture16", "texture17", "texture18", "texture19", "texture20", "texture21", "texture22", "texture23",
            "texture24", "texture25", "texture26", "texture27", "texture28", "texture29", "texture30", "texture31",
    };
}

void grx::grx_texture_set::bind(shader_program_id_t program_id) {
    if (_cached_program != program_id) {
        _cached_program = program_id;

        for (size_t i = 0; i < _textures.size(); ++i) {
            _textures[i].second = static_cast<uniform_id_t>(
                    glGetUniformLocation(static_cast<GLuint>(program_id), textures_uniform_names[i]));
        }
    }

    for (uint i = 0; i < static_cast<uint>(_textures.size()); ++i) {
        auto& [texture, uniform] = _textures[i];

        if (uniform != static_cast<uniform_id_t>(-1) &&
            texture.id != static_cast<texture_id_t>(std::numeric_limits<uint>::max()))
        {
            //DLOG("set uniform {} to {}, texture id: {}", uniform, i, texture.id);
            grx_shader_mgr::set_uniform(uniform, static_cast<int>(i));
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture.id));
        }
    }
}

*/

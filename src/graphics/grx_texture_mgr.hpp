#pragma once

#include <mutex>
#include <core/types.hpp>
#include <core/assert.hpp>
#include "grx_types.hpp"
#include "grx_color_map.hpp"

namespace core {
    class config_manager;
}

namespace grx {

    namespace grx_texture_type {
        enum grx_texture_type : size_t {
            diffuse = 0,
            normal,
            specular,
            types_count
        };
    }

    class grx_texture_set {
    public:
        grx_texture_set(size_t count = 1): _textures(count, core::pair{grx_texture(), static_cast<uniform_id_t>(-1)}) {
            RASSERTF(count < 32, "Wrong textures count {} (Must be < 32)", count);
        }

        grx_texture& get_or_create(size_t type_or_position) {
            RASSERTF(type_or_position < 32, "Wrong texture position {} (Must be < 32)", type_or_position);

            if (_textures.size() <= type_or_position) {
                _textures.resize(type_or_position + 1, core::pair{grx_texture(), static_cast<uniform_id_t>(-1)});
                // Reload uniforms
                _cached_program = static_cast<shader_program_id_t>(-1);
            }

            return _textures[type_or_position].first;
        }

        grx_texture& diffuse() {
            return get_or_create(grx_texture_type::diffuse);
        }

        grx_texture& normal() {
            return get_or_create(grx_texture_type::normal);
        }

        grx_texture& specular() {
            return get_or_create(grx_texture_type::specular);
        }

        [[nodiscard]]
        grx_texture& texture(size_t type_or_position) {
            return _textures.at(type_or_position).first;
        }

        [[nodiscard]]
        const grx_texture& texture(size_t type_or_position) const {
            return _textures.at(type_or_position).first;
        }

        [[nodiscard]]
        const grx_texture& diffuse() const {
            return _textures.at(grx_texture_type::diffuse).first;
        }

        [[nodiscard]]
        const grx_texture& normal() const {
            return _textures.at(grx_texture_type::normal).first;
        }

        [[nodiscard]]
        const grx_texture& specular() const {
            return _textures.at(grx_texture_type::specular).first;
        }

        void bind(shader_program_id_t program_id);

    private:
        core::vector<core::pair<grx_texture, uniform_id_t>> _textures;
        shader_program_id_t        _cached_program = static_cast<shader_program_id_t>(-1);
    };


    class grx_texture_mgr {
    public:
        core::optional<grx_texture> load(core::string_view path);
        core::optional<grx_texture> load(const core::config_manager& config_mgr, core::string_view path);

        grx_texture load_unwrap(core::string_view path);
        grx_texture load_unwrap(const core::config_manager& config_mgr, core::string_view path);

    private:
        std::mutex _texture_ids_mutex;
        core::hash_map<core::string, grx_texture> _texture_ids;
    };

} // namespace grx

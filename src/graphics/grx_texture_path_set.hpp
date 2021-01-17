#pragma once

#include "core/resource_mgr_base.hpp"
#include <core/types.hpp>
#include <core/option_array.hpp>
#include <core/config_manager.hpp>

class aiMaterial;
class aiScene;

namespace grx
{

enum class grx_texture_set_tag : size_t { diffuse = 0, normal, specular, grx_texture_set_tag_count };

class grx_texture_path_set
    : public core::option_array<core::resource_path_t,
                                grx_texture_set_tag,
                                static_cast<size_t>(
                                    grx_texture_set_tag::grx_texture_set_tag_count)> {
public:
    void serialize(core::vector<core::byte>& s) const {
        core::serialize(this->options(), s);
    }

    void deserialize(core::span<const core::byte>& d) {
        core::deserialize(this->options(), d);
    }

    static grx_texture_path_set
    from_assimp(const core::string&       path_dir_key,
                const core::string&       path_prefix,
                const aiMaterial*         assimp_material,
                const core::string&       mgr_tag           = "default",
                core::load_significance_t load_significance = core::load_significance_t::medium);
};

core::vector<grx_texture_path_set> get_texture_paths_from_assimp(
    const core::string&       path_dir_key,
    const core::string&       path_prefix,
    const aiScene*            assimp_scene,
    const core::string&       mgr_tag = "default",
    core::load_significance_t load_significance = core::load_significance_t::medium);

} // namespace grx

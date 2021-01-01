#pragma once

#include <core/types.hpp>
#include <core/option_array.hpp>
#include <core/config_manager.hpp>

class aiMaterial;
class aiScene;

namespace grx
{

enum class grx_texture_set_tag : size_t { diffuse = 0, normal, specular, grx_texture_set_tag_count };

class grx_texture_path_set
    : public core::option_array<core::cfg_path,
                                grx_texture_set_tag,
                                static_cast<size_t>(
                                    grx_texture_set_tag::grx_texture_set_tag_count)> {
public:
    static grx_texture_path_set from_assimp(const core::string& path_dir_key,
                                            const core::string& path_prefix,
                                            const aiMaterial*   assimp_material);
};

core::vector<grx_texture_path_set>
get_texture_paths_from_assimp(const core::string& path_dir_key, const core::string& path_prefix, const aiScene* assimp_scene);

} // namespace grx

#include "grx_texture_path_set.hpp"

#include <core/container_extensions.hpp>
#include <assimp/material.h>
#include <assimp/scene.h>

using namespace core;

namespace grx
{

grx_texture_path_set grx_texture_path_set::from_assimp(const string&     path_dir_key,
                                                       const string&     path_prefix,
                                                       const aiMaterial* material) {
    grx_texture_path_set set;

    auto try_extract = [&](const aiMaterial* material, aiTextureType texture_type) -> optional<cfg_path> {
        if (material->GetTextureCount(texture_type)) {
            aiString texture_path;
            if (material->GetTexture(texture_type, 0, &texture_path) == AI_SUCCESS)
                return cfg_path(path_dir_key, path_prefix / string(texture_path.C_Str(), texture_path.length));
        }
        return nullopt;
    };

    set.set<grx_texture_set_tag::diffuse> (try_extract(material, aiTextureType_DIFFUSE));
    set.set<grx_texture_set_tag::normal>  (try_extract(material, aiTextureType_NORMALS));
    set.set<grx_texture_set_tag::specular>(try_extract(material, aiTextureType_SPECULAR));

    return set;
}

vector<grx_texture_path_set>
get_texture_paths_from_assimp(const string& path_dir_key, const string& path_prefix, const aiScene* assimp_scene) {
    vector<grx_texture_path_set> paths;

    for (auto material : span(assimp_scene->mMaterials, assimp_scene->mNumMaterials))
        paths.emplace_back(grx_texture_path_set::from_assimp(path_dir_key, path_prefix, material));

    return paths;
}

} // namespace grx

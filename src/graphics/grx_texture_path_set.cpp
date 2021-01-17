#include "grx_texture_path_set.hpp"

#include <core/container_extensions.hpp>
#include <assimp/material.h>
#include <assimp/scene.h>

using namespace core;

namespace grx
{

grx_texture_path_set
grx_texture_path_set::from_assimp(const string&             path_dir_key,
                                  const string&             path_prefix,
                                  const aiMaterial*         material,
                                  const core::string&       mgr_tag,
                                  core::load_significance_t load_significance) {
    grx_texture_path_set set;

    auto try_extract = [&](const aiMaterial* material, aiTextureType texture_type) -> optional<resource_path_t> {
        optional<resource_path_t> result;
        if (material->GetTextureCount(texture_type)) {
            aiString texture_path;
            if (material->GetTexture(texture_type, 0, &texture_path) == AI_SUCCESS)
                result = resource_path_t{
                    mgr_tag,
                    cfg_path(path_dir_key,
                             path_prefix / string(texture_path.C_Str(), texture_path.length)),
                    load_significance};
        }
        if (!result) {
            switch (texture_type) {
            case aiTextureType_DIFFUSE:
                result = resource_path_t{mgr_tag,
                                         cfg_path("textures_dir", "basic/dummy_diffuse.png"),
                                         load_significance};
                break;
            case aiTextureType_NORMALS:
                result = resource_path_t{mgr_tag,
                                         cfg_path("textures_dir", "basic/dummy_normal.png"),
                                         load_significance};
                break;
            default: break;
            }
        }

        return result;
    };

    set.set<grx_texture_set_tag::diffuse> (try_extract(material, aiTextureType_DIFFUSE));
    set.set<grx_texture_set_tag::normal>  (try_extract(material, aiTextureType_NORMALS));
    set.set<grx_texture_set_tag::specular>(try_extract(material, aiTextureType_SPECULAR));

    return set;
}

vector<grx_texture_path_set>
get_texture_paths_from_assimp(const string&             path_dir_key,
                              const string&             path_prefix,
                              const aiScene*            assimp_scene,
                              const core::string&       mgr_tag,
                              core::load_significance_t load_significance) {
    vector<grx_texture_path_set> paths;

    for (auto material : span(assimp_scene->mMaterials, assimp_scene->mNumMaterials))
        paths.emplace_back(grx_texture_path_set::from_assimp(path_dir_key, path_prefix, material, mgr_tag, load_significance));

    return paths;
}

} // namespace grx

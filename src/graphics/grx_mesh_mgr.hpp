#pragma once

#include <mutex>
#include "grx_mesh.hpp"

#define MAX_BONES_COUNT 128

namespace core
{
class config_manager;
}

class aiScene;

namespace grx
{
using core::string_view;

// TODO: private constructors, create_shared member function

class grx_mesh_mgr {
private:
    static void load_anim(const aiScene* scene, grx_bone_data& bone_data);

public:
    grx_mesh_mgr(const core::config_manager& config_manager);

    shared_ptr<grx_mesh> load(string_view path, bool instanced = false);

    static void anim_traverse(core::span<glm::mat4> output_bone_transforms,
                              grx_bone_data&        bone_data,
                              const grx_animation&  anim,
                              double                time,
                              const bone_node*      node,
                              const glm::mat4&      parent_transform = glm::mat4(1.f));

private:
    static shared_ptr<grx_mesh> load_mesh(string_view path, bool instanced, grx_texture_mgr* texture_mgr);

private:
    std::mutex                             _meshes_mutex;
    hash_map<string, shared_ptr<grx_mesh>> _meshes;
    core::shared_ptr<grx_texture_mgr>      _texture_mgr;
    string                                 _models_dir;
};

} // namespace grx


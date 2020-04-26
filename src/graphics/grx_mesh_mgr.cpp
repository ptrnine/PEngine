#include "grx_mesh_mgr.hpp"

#include <GL/glew.h>

#include <glm/gtc/type_ptr.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/material.h>

#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/config_manager.hpp>
#include <core/container_extensions.hpp>
#include <core/math.hpp>

#include "grx_camera.hpp"
#include "grx_shader_mgr.hpp"
#include "grx_utils.hpp"

using core::operator/;

namespace grx {
    inline glm::mat4 to_glm(const aiMatrix4x4& m) {
        glm::mat4 to;

        to[0][0] = m.a1; to[0][1] = m.b1; to[0][2] = m.c1; to[0][3] = m.d1;
        to[1][0] = m.a2; to[1][1] = m.b2; to[1][2] = m.c2; to[1][3] = m.d2;
        to[2][0] = m.a3; to[2][1] = m.b3; to[2][2] = m.c3; to[2][3] = m.d3;
        to[3][0] = m.a4; to[3][1] = m.b4; to[3][2] = m.c4; to[3][3] = m.d4;

        return to;
    }

    void append_transform_traverse(
            const core::vector<grx_mesh_entry>& mesh_entry,
            const aiScene* scene,
            aiNode* node,
            vbo_vector_vec3f& positions,
            int level = 0
    ) {
        for (int i = 0; i < level; ++i)
            std::cout << "    ";
        std::cout << level << " : " << node->mName.data << std::endl;

        for (size_t i = 0; i < node->mNumChildren; ++i)
            append_transform_traverse(mesh_entry, scene, node->mChildren[i], positions, level + 1);
    }

    template <typename VboT>
    core::pair<VboT, core::vector<grx_mesh_entry>> load_mesh_vbo(const aiScene* scene) {
        VboT mesh_vbo;

        DLOG("Meshes count:     {}", scene->mNumMeshes);
        DLOG("Textures count:   {}", scene->mNumTextures);
        DLOG("Materials count:  {}", scene->mNumMaterials);

        uint vertices_count = 0;
        uint indices_count = 0;

        core::vector<grx_mesh_entry> mesh_entries(scene->mNumMeshes);
        for (size_t i = 0; i < mesh_entries.size(); ++i) {
            mesh_entries[i] = grx_mesh_entry{
                scene->mMeshes[i]->mNumFaces * 3,  // Indices count
                scene->mMeshes[i]->mMaterialIndex,
                vertices_count,
                indices_count
            };
            vertices_count += scene->mMeshes[i]->mNumVertices;
            indices_count  += mesh_entries[i].indices_count;
        }

        DLOG("Total vertices: {}", vertices_count);
        DLOG("Total indices:  {}", indices_count);

        vbo_vector_indices indices;
        vbo_vector_vec3f   positions;
        vbo_vector_vec3f   normals;
        vbo_vector_vec2f   uvs;
        vbo_vector_vec3f   tangents;
        vbo_vector_vec3f   bitangents;

        auto vector_vec3f_append = [](vbo_vector_vec3f& vec, const aiVector3D* data, size_t count) {
            auto old_size = vec.size();
            vec.resize(old_size + count);
            memcpy(vec.data() + old_size, data, sizeof(core::vec3f) * count);
        };

        for (size_t i = 0; i < scene->mNumMeshes; ++i) {
            auto mesh = scene->mMeshes[i];

            DLOG("Load mesh [{}]...", i);
            DLOG("Name:                 {}", mesh->mName.C_Str());
            DLOG("Vertices count:       {}", mesh->mNumVertices);
            DLOG("Faces count:          {}", mesh->mNumFaces);
            DLOG("Has positions:        {}", mesh->HasPositions());
            DLOG("Has faces:            {}", mesh->HasFaces());
            DLOG("Has normals:          {}", mesh->HasNormals());
            DLOG("Has tan and bitan:    {}", mesh->HasTangentsAndBitangents());
            DLOG("Color channels count: {}", mesh->GetNumColorChannels());
            DLOG("UV channels count:    {}", mesh->GetNumUVChannels());

            if (!mesh->HasTangentsAndBitangents()) {
                LOG_WARNING("Mesh [{}] has not tangents, skipping", mesh->mName.C_Str());
                continue;
            }

            {
                RASSERTF(mesh->HasFaces(), "{}", "Mesh loader doesn't support meshes without faces");
                auto old_size = indices.size();
                indices.resize(old_size + mesh->mNumFaces * 3);
                for (size_t j = 0; j < mesh->mNumFaces; ++j) {
                    auto& face = mesh->mFaces[j];
                    ASSERTF(face.mNumIndices == 3, "{}", "Mesh loader support 3 indices per face only");
                    indices[old_size++] = face.mIndices[0];
                    indices[old_size++] = face.mIndices[1];
                    indices[old_size++] = face.mIndices[2];
                }
            }

            vector_vec3f_append(positions,  mesh->mVertices,   mesh->mNumVertices);
            vector_vec3f_append(normals,    mesh->mNormals,    mesh->mNumVertices);
            vector_vec3f_append(tangents,   mesh->mTangents,   mesh->mNumVertices);
            vector_vec3f_append(bitangents, mesh->mBitangents, mesh->mNumVertices);

            {
                if (mesh->GetNumUVChannels() > 0 && mesh->HasTextureCoords(0)) {
                    auto old_size = uvs.size();
                    uvs.resize(old_size + mesh->mNumVertices);
                    for (size_t k = 0; k < mesh->mNumVertices; ++k)
                        uvs[old_size++] = core::vec2f{mesh->mTextureCoords[0][k].x, mesh->mTextureCoords[0][k].y};
                } else {
                    auto old_size = uvs.size();
                    uvs.resize(old_size + mesh->mNumVertices);
                    std::fill(uvs.begin() + static_cast<vbo_vector_vec2f::difference_type>(old_size),
                            uvs.end(), core::vec{0.0f, 0.0f});
                }
            }
        }



//        core::hash_set<core::string> boneset;
//        for (size_t i = 0; i < scene->mNumMeshes; ++i) {
//            auto mesh = scene->mMeshes[i];
//
//            for (size_t k = 0; k < mesh->mNumBones; ++k) {
//                auto bone = mesh->mBones[k];
//
//                if (boneset.emplace(bone->mName.data).second) {
//                    for (size_t q = 0; q < bone->mNumWeights; ++q) {
//                        auto mat = to_glm(bone->mOffsetMatrix);
//                        auto vec = to_glm(positions[bone->mWeights[q].mVertexId]);
//                        auto k = mat * glm::vec4(vec, 1.f);
//                        positions[bone->mWeights[q].mVertexId] = core::from_glm_vec(k).xyz();
//                    }
//                }
//            }
//        }

        //append_transform_traverse(mesh_entries, scene, scene->mRootNode, positions);

        mesh_vbo.template set_data<mesh_vbo_types::INDEX_BUF    >(indices);
        mesh_vbo.template set_data<mesh_vbo_types::POSITION_BUF >(positions);
        mesh_vbo.template set_data<mesh_vbo_types::UV_BUF       >(uvs);
//        mesh_vbo.template set_data<mesh_vbo_types::NORMAL_BUF   >(normals);
//        mesh_vbo.template set_data<mesh_vbo_types::TANGENT_BUF  >(tangents);
//        mesh_vbo.template set_data<mesh_vbo_types::BITANGENT_BUF>(bitangents);

        return { std::move(mesh_vbo), std::move(mesh_entries) };
    }

    core::vector<grx_texture_set> load_texture_sets(
            const aiScene* scene,
            core::string_view dir,
            grx_texture_mgr& texture_mgr
    ) {
        core::vector<grx_texture_set> texture_set(scene->mNumMaterials);

        for (size_t i = 0; i < scene->mNumMaterials; ++i) {
            auto material = scene->mMaterials[i];
            DLOG("Load material [{}]", material->GetName().data);

            if (material->GetTextureCount(aiTextureType_DIFFUSE)) {
                aiString texture_path;
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path) == AI_SUCCESS) {
                    DLOG("\tLoad diffuse '{}'", texture_path.C_Str());
                    if (auto texture = texture_mgr.load(dir / core::string(texture_path.C_Str())))
                        texture_set[i].diffuse() = *texture;
                    else
                        LOG_WARNING("Can't load texture '{}'", dir / core::string(texture_path.C_Str()));
                }
            }
            if (material->GetTextureCount(aiTextureType_NORMALS)) {
                aiString texture_path;
                if (material->GetTexture(aiTextureType_NORMALS, 0, &texture_path) == AI_SUCCESS) {
                    DLOG("\tLoad normal '{}'", texture_path.C_Str());
                    if (auto texture = texture_mgr.load(dir / core::string(texture_path.C_Str())))
                        texture_set[i].normal() = *texture;
                    else
                        LOG_WARNING("Can't load texture '{}'", dir / core::string(texture_path.C_Str()));
                }
            }
            if (material->GetTextureCount(aiTextureType_SPECULAR)) {
                aiString texture_path;
                if (material->GetTexture(aiTextureType_SPECULAR, 0, &texture_path) == AI_SUCCESS) {
                    DLOG("\tLoad specular '{}'", texture_path.C_Str());
                    if (auto texture = texture_mgr.load(dir / core::string(texture_path.C_Str())))
                        texture_set[i].specular() = *texture;
                    else
                        LOG_WARNING("Can't load texture '{}'", dir / core::string(texture_path.C_Str()));
                }
            }
        }

        return texture_set;
    }

    template <typename VboType>
    auto load_bones(
            const aiScene* scene,
            VboType& vbo,
            const core::vector<grx_mesh_entry>& mesh_entries,
            size_t vertices_count
    ) {
        vbo_vector_bone bones(vertices_count, vbo_vector_bone::value_type());
        core::hash_map<core::string, uint> bone_map;
        core::vector<glm::mat4> offsets;
        core::vector<glm::mat4> transforms;

        for (size_t i = 0; i < scene->mNumMeshes; ++i) {
            auto mesh = scene->mMeshes[i];

            for (size_t j = 0; j < mesh->mNumBones; ++j) {
                auto bone = mesh->mBones[j];
                auto [position, was_inserted] = bone_map.emplace(bone->mName.data, offsets.size());

                if (was_inserted) {
                    DLOG("Load bone [{}]", bone->mName.data);

                    offsets   .emplace_back(to_glm(bone->mOffsetMatrix));
                    transforms.emplace_back(glm::mat4(1.f));
                }

                for (size_t k = 0; k < bone->mNumWeights; ++k) {
                    auto vertex_id = mesh_entries[i].start_vertex_pos + bone->mWeights[k].mVertexId;
                    bones[vertex_id].append(position->second, bone->mWeights[k].mWeight);
                }
            }
        }

        vbo.template set_data<mesh_vbo_skeleton_types::BONE_BUF>(bones);
        return core::tuple{std::move(bone_map), std::move(offsets), std::move(transforms)};
    }

    void load_node_iter(bone_node* dst, aiNode* src, size_t level = 0) {
        LOG("Node [{}]", src->mName.data);
        dst->name = src->mName.data;
        dst->transform = to_glm(src->mTransformation);

        //LOG("Position: {}", core::from_glm_vec(src->mTransformation));

        dst->children.resize(src->mNumChildren);

        for (size_t i = 0; i < src->mNumChildren; ++i) {
            dst->children[i] = core::make_unique<bone_node>();
            load_node_iter(dst->children[i].get(), src->mChildren[i], level + 1);
        }
    }

    void load_anim(const aiScene* scene, grx_bone_data& bone_data) {
        bone_data.root_node = core::make_unique<bone_node>();
        load_node_iter(bone_data.root_node.get(), scene->mRootNode);

        DLOG("Animations count: {}", scene->mNumAnimations);
        for (size_t i = 0; i < scene->mNumAnimations; ++i) {
            auto anim = scene->mAnimations[i];
            auto [pos, was_inserted] = bone_data.animations.emplace(core::string(anim->mName.data), grx_animation());

            DLOG("Animation name: {}", anim->mName.data);

            if (was_inserted) {
                pos->second.duration         = anim->mDuration;
                pos->second.ticks_per_second = anim->mTicksPerSecond;

                DLOG("Channels count: {}", anim->mNumChannels);
                for (size_t j = 0; j < anim->mNumChannels; ++j) {
                    auto channel = anim->mChannels[j];

                    DLOG("Channel name: {}", channel->mNodeName.data);

                    auto [pos2, was_inserted2] = pos->second.channels.emplace(
                            core::string(channel->mNodeName.data), anim_channel());

                    if (was_inserted2) {
                        auto& src_channel = pos2->second;

                        src_channel.position_keys.resize(channel->mNumPositionKeys);
                        src_channel.scaling_keys .resize(channel->mNumScalingKeys);
                        src_channel.rotation_keys.resize(channel->mNumRotationKeys);

                        for (size_t k = 0; k < channel->mNumPositionKeys; ++k) {
                            src_channel.position_keys[k].time = channel->mPositionKeys[k].mTime;
                            src_channel.position_keys[k].value = core::vec{
                                    channel->mPositionKeys[k].mValue.x,
                                    channel->mPositionKeys[k].mValue.y,
                                    channel->mPositionKeys[k].mValue.z
                            };
                        }
                        for (size_t k = 0; k < channel->mNumScalingKeys; ++k) {
                            src_channel.scaling_keys[k].time = channel->mScalingKeys[k].mTime;
                            src_channel.scaling_keys[k].value = core::vec{
                                    channel->mScalingKeys[k].mValue.x,
                                    channel->mScalingKeys[k].mValue.y,
                                    channel->mScalingKeys[k].mValue.z
                            };
                        }
                        for (size_t k = 0; k < channel->mNumRotationKeys; ++k) {
                            src_channel.rotation_keys[k].time  = channel->mRotationKeys[k].mTime;
                            src_channel.rotation_keys[k].value = glm::quat(
                                    (channel->mRotationKeys[k].mValue.w),
                                    (channel->mRotationKeys[k].mValue.x),
                                    (channel->mRotationKeys[k].mValue.y),
                                    (channel->mRotationKeys[k].mValue.z)
                            );
                        }
                    }
                }
            }
        }
    }

    bool is_scene_has_bones(const aiScene* scene) {
        for (size_t i = 0; i < scene->mNumMeshes; ++i)
            if (scene->mMeshes[i]->HasBones())
                return true;
        return false;
    }

    void anim_traverse(
            grx_bone_data& bone_data,
            const grx_animation& anim,
            double time,
            const bone_node* node,
            const glm::mat4& parent_transform = glm::mat4(1.f)
    ) {
        auto calc_scaling = [](double time, const anim_channel& c) -> glm::mat4 {
            if (c.scaling_keys.size() == 1)
                return glm::scale(glm::mat4(1.f), to_glm(c.scaling_keys.front().value));

            size_t scaling_idx = std::numeric_limits<size_t>::max();
            for (size_t i = 0; i < c.scaling_keys.size() - 1; ++i) {
                if (time < c.scaling_keys[i + 1].time) {
                    scaling_idx = i;
                    break;
                }
            }
            size_t next_scaling_idx = scaling_idx + 1;

            ASSERT(scaling_idx != std::numeric_limits<size_t>::max());

            auto delta_time = c.scaling_keys[next_scaling_idx].time - c.scaling_keys[scaling_idx].time;
            auto factor     = (time - c.scaling_keys[scaling_idx].time) / delta_time;

            ASSERT(factor >= 0.f && factor <= 1.f);

            auto vec = core::lerp(c.scaling_keys[scaling_idx].value,
                              c.scaling_keys[next_scaling_idx].value,
                              static_cast<float>(factor));
            return glm::scale(glm::mat4(1.f), to_glm(vec));
        };

        auto calc_rotation = [](double time, const anim_channel& c) -> glm::mat4 {
            if (c.rotation_keys.size() == 1)
                return glm::mat4(c.rotation_keys.front().value);

            size_t idx = std::numeric_limits<size_t>::max();
            for (size_t i = 0; i < c.rotation_keys.size() - 1; ++i) {
                if (time < c.rotation_keys[i + 1].time) {
                    idx = i;
                    break;
                }
            }
            size_t next_idx = idx + 1;

            ASSERT(idx != std::numeric_limits<size_t>::max());
            auto delta_time = c.rotation_keys[next_idx].time - c.rotation_keys[idx].time;
            auto factor     = (time - c.rotation_keys[idx].time) / delta_time;

            ASSERT(factor >= 0.f && factor <= 1.f);

            return glm::mat4_cast(glm::normalize(
                    glm::slerp(c.rotation_keys[idx].value, c.rotation_keys[next_idx].value, static_cast<float>(factor))));
        };

        auto calc_position = [](double time, const anim_channel& c) -> glm::mat4 {
            if (c.position_keys.size() == 1)
                return glm::translate(glm::mat4(1.f), to_glm(c.position_keys.front().value));

            size_t idx = std::numeric_limits<size_t>::max();
            for (size_t i = 0; i < c.position_keys.size() - 1; ++i) {
                if (time < c.position_keys[i + 1].time) {
                    idx = i;
                    break;
                }
            }
            size_t next_idx = idx + 1;

            ASSERT(idx != std::numeric_limits<size_t>::max());
            auto delta_time = c.position_keys[next_idx].time - c.position_keys[idx].time;
            auto factor     = (time - c.position_keys[idx].time) / delta_time;

            ASSERT(factor >= 0.f && factor <= 1.f);

            auto vec = core::lerp(
                    c.position_keys[idx].value, c.position_keys[next_idx].value, static_cast<float>(factor));
            return glm::translate(glm::mat4(1.f), to_glm(vec));
        };

        auto channel_pos = anim.channels.find(node->name);

        glm::mat4 transform(node->transform);

        if (channel_pos != anim.channels.end()) {
            auto scaling     = calc_scaling (time, channel_pos->second);
            auto rotation    = calc_rotation(time, channel_pos->second);
            auto translation = calc_position(time, channel_pos->second);

            transform = translation * scaling * rotation;
        }

        glm::mat4 global_transform = parent_transform * transform;
        auto transform_idx_pos = bone_data.bone_map.find(node->name);
        if (transform_idx_pos != bone_data.bone_map.end()) {
            bone_data.final_transforms[transform_idx_pos->second] =
                    bone_data.global_inverse_transform * global_transform *
                    bone_data.offsets[transform_idx_pos->second];
        }

        for (auto& child : node->children)
            anim_traverse(bone_data, anim, time, child.get(), global_transform);
    }
} // namespace grx

core::string collada_prepare(core::string_view path) {
    std::ifstream ifs(core::string(path), std::ios::in | std::ios::binary);

    if (!ifs.is_open())
        return "";

    ifs.seekg(0, std::ios::end);
    auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    auto size = static_cast<size_t>(end - ifs.tellg());

    core::string data;
    data.resize(size);
    ifs.read(data.data(), static_cast<std::streamsize>(size));

    return grx_utils::collada_bake_bind_shape_matrix(data);
}

auto grx::grx_mesh_mgr::
load_mesh(core::string_view path, bool instanced) -> core::shared_ptr<grx_mesh>
{
    auto importer = Assimp::Importer();

    const aiScene* scene;

    if (path.ends_with(".dae")) {
        auto data = collada_prepare(path);
        scene = importer.ReadFileFromMemory(data.data(), data.length(),
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_FlipUVs
        );

//        scene = importer.ReadFile(path.data(),
//            aiProcess_Triangulate |
//            aiProcess_GenSmoothNormals |
//            aiProcess_CalcTangentSpace |
//            aiProcess_FlipUVs
//        );
    } else {

        scene = importer.ReadFile(path.data(),
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_FlipUVs
        );
    }

    RASSERTF(scene, "Can't load mesh at path '{}': {}", path, importer.GetErrorString());
    RASSERTF(scene->HasMeshes(), "Scene at path '{}' doesn't have any mesh", path);

    DLOG("Load scene data '{}'", path);

    core::shared_ptr<grx_mesh> mesh;

    if (instanced) {
        auto [mesh_vbo, mesh_entries] = load_mesh_vbo<mesh_instanced_vbo_t>(scene);
        mesh = core::shared_ptr<grx_mesh>(
                new grx_mesh(std::move(mesh_vbo), std::move(mesh_entries)));
    }
    else if (is_scene_has_bones(scene)) {
        auto [mesh_vbo, mesh_entries]    = load_mesh_vbo<mesh_vbo_skeleton_t>(scene);
        auto [bone_map, bone_offsets, bone_transforms] =
                load_bones<mesh_vbo_skeleton_t>(scene, mesh_vbo, mesh_entries,
                mesh_entries.back().start_vertex_pos + mesh_entries.back().indices_count * 3);

        mesh = core::shared_ptr<grx_mesh>(
                new grx_mesh(std::move(mesh_vbo), std::move(mesh_entries)));

        mesh->_bone_data = core::make_unique<grx_bone_data>();
        mesh->_bone_data->bone_map         = std::move(bone_map);
        mesh->_bone_data->offsets          = std::move(bone_offsets);
        mesh->_bone_data->final_transforms = std::move(bone_transforms);
        mesh->_bone_data->global_inverse_transform = glm::inverse(to_glm(scene->mRootNode->mTransformation));

        load_anim(scene, *(mesh->_bone_data));
    }
    else {
        auto [mesh_vbo, mesh_entries] = load_mesh_vbo<mesh_vbo_t>(scene);
        mesh = core::shared_ptr<grx_mesh>(
                new grx_mesh(std::move(mesh_vbo), std::move(mesh_entries)));
    }

    mesh->_texture_sets = load_texture_sets(scene, core::path_eval(path / ".."), _texture_mgr);

    return mesh;
}

auto grx::grx_mesh_mgr::load(core::string_view p, bool instanced) -> core::shared_ptr<grx_mesh> {
    auto path = core::path_eval(p);

    auto [position, was_inserted] = _meshes.emplace(path, nullptr);

    if (was_inserted)
        position->second = load_mesh(path, instanced);

    return position->second;
}

auto grx::grx_mesh_mgr::load(const core::config_manager& cm, core::string_view p, bool instanced)
-> core::shared_ptr<grx_mesh>
{
    return load(cm.entry_dir() / cm.read_unwrap<core::string>("models_dir") / p, instanced);
}

void grx::grx_mesh::draw(const core::shared_ptr<grx_camera>& camera, shader_program_id_t program_id) {
    if (_cached_program != program_id) {
        _cached_program = program_id;
        _cached_uniform_mvp   = static_cast<uniform_id_t>(glGetUniformLocation(static_cast<GLuint>(program_id), "MVP"));
        _cached_uniform_model = static_cast<uniform_id_t>(glGetUniformLocation(static_cast<GLuint>(program_id), "M"));

        if (_bone_data) {
            _bone_data->cached_bone_matrices_uniform = static_cast<uniform_id_t>(
                    glGetUniformLocation(static_cast<GLuint>(program_id), "bone_matrices"));
        }
    }

    if (_cached_uniform_mvp != static_cast<uniform_id_t>(-1))
        grx::grx_shader_mgr::set_uniform(_cached_uniform_mvp, camera->view_projection() * _model_matrices[0]);

    if (_cached_uniform_model != static_cast<uniform_id_t>(-1))
        grx::grx_shader_mgr::set_uniform(_cached_uniform_model, _model_matrices[0]);

    static double glob_time = 0;
    if (_bone_data) {
        if (!_bone_data->animations.empty()) {
            double fact = std::fmod(glob_time += 0.01, _bone_data->animations.begin()->second.duration);
            anim_traverse(
                    *_bone_data,
                    _bone_data->animations.begin()->second,
                    fact,
                    _bone_data->root_node.get());
        }

        glUniformMatrix4fv(
                static_cast<GLint>(_bone_data->cached_bone_matrices_uniform),
                static_cast<GLsizei>(_bone_data->final_transforms.size()),
                GL_FALSE,
                &(_bone_data->final_transforms.front()[0][0]));
    }

    _mesh_vbo.bind_vao();

    for (auto& entry : _mesh_entries) {
        if (entry.material_index != std::numeric_limits<uint>::max())
            _texture_sets[entry.material_index].bind(program_id);

        _mesh_vbo.draw(entry.indices_count, entry.start_vertex_pos, entry.start_index_pos);
    }
}

void grx::grx_mesh::draw_instanced(const core::shared_ptr<grx_camera>& camera, shader_program_id_t program_id) {
    auto& vbo_tuple = _mesh_vbo.cast<mesh_instanced_vbo_t>();

    auto vp   = camera->view_projection();
    auto mvps = core::vector<glm::mat4>(_model_matrices.size());

    for (size_t i = 0; i < _model_matrices.size(); ++i)
        mvps[i] = vp * _model_matrices[i];

    vbo_tuple.set_data<mesh_vbo_types::MVP_MAT>(mvps);
    vbo_tuple.set_data<mesh_vbo_types::MODEL_MAT>(_model_matrices);

    vbo_tuple.bind_vao();

    for (auto& entry : _mesh_entries) {
        if (entry.material_index != std::numeric_limits<uint>::max())
            _texture_sets[entry.material_index].bind(program_id);

        vbo_tuple.draw_instanced(
                _model_matrices.size(),
                entry.indices_count,
                entry.start_vertex_pos,
                entry.start_index_pos);
    }
}

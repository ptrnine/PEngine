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
#include "grx_debug.hpp"

using namespace core;

namespace grx {
    inline glm::mat4 to_glm(const aiMatrix4x4& m) {
        glm::mat4 to;

        to[0][0] = m.a1; to[0][1] = m.b1; to[0][2] = m.c1; to[0][3] = m.d1;
        to[1][0] = m.a2; to[1][1] = m.b2; to[1][2] = m.c2; to[1][3] = m.d2;
        to[2][0] = m.a3; to[2][1] = m.b3; to[2][2] = m.c3; to[2][3] = m.d3;
        to[3][0] = m.a4; to[3][1] = m.b4; to[3][2] = m.c4; to[3][3] = m.d4;

        return to;
    }

    inline core::vec3f to_vec3f(const aiVector3D& v) {
        return core::vec3f{v.x, v.y, v.z};
    }

    mesh_data_basic mesh_extract_basic_data(const aiScene* scene) {
        DLOG("Meshes count:     {}", scene->mNumMeshes);
        DLOG("Textures count:   {}", scene->mNumTextures);
        DLOG("Materials count:  {}", scene->mNumMaterials);

        uint vertices_count = 0;
        uint indices_count  = 0;
        uint faces_count    = 0;

        core::vector<grx_mesh_entry> mesh_entries(scene->mNumMeshes);

        auto meshes = core::span(scene->mMeshes, scene->mNumMeshes);
        for (auto& [dst, src] : core::zip_view(mesh_entries, meshes)) {
            dst = grx_mesh_entry{
                src->mNumFaces * 3,  // Indices count
                src->mNumVertices,
                src->mMaterialIndex,
                vertices_count,
                indices_count,
                grx_aabb{to_vec3f(src->mAABB.mMin), to_vec3f(src->mAABB.mMax)}
            };
            vertices_count += src->mNumVertices;
            indices_count  += dst.indices_count;
            faces_count    += src->mNumFaces;
        }

        DLOG("Total vertices: {}", vertices_count);
        DLOG("Total indices:  {}", indices_count);
        DLOG("Total faces:    {}", faces_count);

        RASSERTF(faces_count * 3 == indices_count, "{}", "May be triangulate failed?");

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

        for (auto mesh : core::span(scene->mMeshes, scene->mNumMeshes)) {
            DLOG("Load mesh [{}]...", mesh->mName.C_Str());
            DLOG("Vertices count:       {}", mesh->mNumVertices);
            DLOG("Faces count:          {}", mesh->mNumFaces);
            DLOG("Has positions:        {}", mesh->HasPositions());
            DLOG("Has faces:            {}", mesh->HasFaces());
            DLOG("Has normals:          {}", mesh->HasNormals());
            DLOG("Has tan and bitan:    {}", mesh->HasTangentsAndBitangents());
            DLOG("Color channels count: {}", mesh->GetNumColorChannels());
            DLOG("UV channels count:    {}", mesh->GetNumUVChannels());
            DLOG("AABB: min: {.3} max: {.3}", to_vec3f(mesh->mAABB.mMin), to_vec3f(mesh->mAABB.mMax));

            if (!mesh->HasTangentsAndBitangents()) {
                LOG_WARNING("Mesh [{}] has not tangents, skipping", mesh->mName.C_Str());
                continue;
            }

            {
                RASSERTF(mesh->HasFaces(), "{}", "Mesh loader doesn't support meshes without faces");
                auto old_size = indices.size();
                indices.resize(old_size + mesh->mNumFaces * 3);
                for (auto& face : core::span(mesh->mFaces, mesh->mNumFaces)) {
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
                    for (auto& uv : core::span(mesh->mTextureCoords[0], mesh->mNumVertices))
                        uvs[old_size++] = core::vec{uv.x, uv.y};
                } else {
                    auto old_size = uvs.size();
                    uvs.resize(old_size + mesh->mNumVertices);
                    std::fill(uvs.begin() + static_cast<vbo_vector_vec2f::difference_type>(old_size),
                            uvs.end(), core::vec{0.0f, 0.0f});
                }
            }
        }

        return {
            std::move(mesh_entries),
            std::move(indices),
            std::move(positions),
            std::move(uvs),
            std::move(normals),
            std::move(tangents),
            std::move(bitangents)
        };
    }

    void load_node_iter(bone_node* dst, aiNode* src) {
        dst->name = src->mName.data;
        dst->transform = to_glm(src->mTransformation);

        dst->children.resize(src->mNumChildren);
        auto children = core::span(src->mChildren, src->mNumChildren);

        for (auto& [dst1, src1] : core::zip_view(dst->children, children)) {
            dst1 = core::make_unique<bone_node>();
            load_node_iter(dst1.get(), src1);
        }
    }

    mesh_bone_data
    mesh_extract_bone_data(const aiScene* scene, const core::vector<grx_mesh_entry>& mesh_entries) {
        auto skeleton_tree = core::make_unique<bone_node>();
        load_node_iter(skeleton_tree.get(), scene->mRootNode);

        auto vertices_count = mesh_entries.back().start_vertex_pos + mesh_entries.back().vertices_count;
        vbo_vector_bone bones(vertices_count, vbo_vector_bone::value_type());

        core::hash_map<core::string, uint> bone_map;
        core::vector<grx_aabb>  aabbs;
        core::vector<glm::mat4> offsets;
        core::vector<glm::mat4> transforms;

        auto src_meshes = core::span(scene->mMeshes, scene->mNumMeshes);

        for (auto& [mesh, i] : core::value_index_view(src_meshes)) {
            for (auto bone : core::span(mesh->mBones, mesh->mNumBones)) {
                auto [position, was_inserted] = bone_map.emplace(bone->mName.data, offsets.size());

                if (was_inserted) {
                    DLOG("Load bone [{}]", bone->mName.data);

                    offsets   .emplace_back(to_glm(bone->mOffsetMatrix));
                    transforms.emplace_back(glm::mat4(1.f));
                    aabbs     .emplace_back(grx_aabb::maximized());
                }

                auto& aabb = aabbs[position->second];

                for (auto& weight : core::span(bone->mWeights, bone->mNumWeights)) {
                    auto vertex_id = mesh_entries[i].start_vertex_pos + weight.mVertexId;
                    bones[vertex_id].append(position->second, weight.mWeight);

                    auto vertex = to_vec3f(mesh->mVertices[weight.mVertexId]);
                    aabb.merge({vertex, vertex});
                }
            }
        }

        return {
            std::move(bones),
            std::move(skeleton_tree),
            std::move(bone_map),
            std::move(aabbs),
            std::move(offsets),
            std::move(transforms)
        };
    }

    core::hash_map<core::string, grx_animation>
    mesh_extract_animations(const aiScene* scene) {
        core::hash_map<core::string, grx_animation> animations;

        DLOG("Animations count: {}", scene->mNumAnimations);

        for (auto anim : core::span(scene->mAnimations, scene->mNumAnimations)) {
            auto [pos, was_inserted] = animations.emplace(core::string(anim->mName.data), grx_animation());

            DLOG("Animation name: {}", anim->mName.data);

            if (was_inserted) {
                pos->second.duration         = anim->mDuration;
                pos->second.ticks_per_second = anim->mTicksPerSecond;

                DLOG("Channels count: {}", anim->mNumChannels);

                for (auto channel : core::span(anim->mChannels, anim->mNumChannels)) {
                    DLOG("Channel name: {}", channel->mNodeName.data);

                    auto [pos2, was_inserted2] = pos->second.channels.emplace(
                            core::string(channel->mNodeName.data), anim_channel());

                    if (was_inserted2) {
                        auto& src_channel = pos2->second;

                        src_channel.position_keys.resize(channel->mNumPositionKeys);
                        src_channel.scaling_keys .resize(channel->mNumScalingKeys);
                        src_channel.rotation_keys.resize(channel->mNumRotationKeys);

                        auto src_position_keys = core::span(channel->mPositionKeys, channel->mNumPositionKeys);
                        auto src_scaling_keys  = core::span(channel->mScalingKeys, channel->mNumScalingKeys);
                        auto src_rotation_keys = core::span(channel->mRotationKeys, channel->mNumRotationKeys);

                        for (auto& [dst, src] : core::zip_view(src_channel.position_keys, src_position_keys)) {
                            dst.time  = src.mTime;
                            dst.value = core::vec{src.mValue.x, src.mValue.y, src.mValue.z};
                        }
                        for (auto& [dst, src] : core::zip_view(src_channel.scaling_keys, src_scaling_keys)) {
                            dst.time  = src.mTime;
                            dst.value = core::vec{src.mValue.x, src.mValue.y, src.mValue.z};
                        }
                        for (auto& [dst, src] : core::zip_view(src_channel.rotation_keys, src_rotation_keys)) {
                            dst.time  = src.mTime;
                            dst.value = glm::quat(src.mValue.w, src.mValue.x, src.mValue.y, src.mValue.z);
                        }
                    }
                }
            }
        }

        return animations;
    }

    vector<texture_path_pack>
    mesh_extract_texture_paths(const aiScene* scene, const string& dir) {
        auto paths_pack    = vector<texture_path_pack>(scene->mNumMaterials);
        auto src_materials = span(scene->mMaterials, scene->mNumMaterials);

        auto try_extract = [&](aiMaterial* material, aiTextureType texture_type) -> optional<string> {
            if (material->GetTextureCount(texture_type)) {
                aiString texture_path;
                if (material->GetTexture(texture_type, 0, &texture_path) == AI_SUCCESS)
                    return dir / texture_path.C_Str();
            }
            return nullopt;
        };

        for (auto& [dst_paths, material] : zip_view(paths_pack, src_materials)) {
            dst_paths.diffuse  = try_extract(material, aiTextureType_DIFFUSE);
            dst_paths.normal   = try_extract(material, aiTextureType_NORMALS);
            dst_paths.specular = try_extract(material, aiTextureType_SPECULAR);
        }

        return paths_pack;
    }

    template <typename VboT>
    core::pair<VboT, core::vector<grx_mesh_entry>> load_mesh_vbo(const aiScene* scene) {
        VboT mesh_vbo;

        DLOG("Meshes count:     {}", scene->mNumMeshes);
        DLOG("Textures count:   {}", scene->mNumTextures);
        DLOG("Materials count:  {}", scene->mNumMaterials);

        uint vertices_count = 0;
        uint indices_count  = 0;
        uint faces_count    = 0;

        core::vector<grx_mesh_entry> mesh_entries(scene->mNumMeshes);
        auto meshes = core::span(scene->mMeshes, scene->mNumMeshes);
        for (auto& [dst, src] : core::zip_view(mesh_entries, meshes)) {
            dst = grx_mesh_entry{
                src->mNumFaces * 3,  // Indices count
                src->mNumVertices,
                src->mMaterialIndex,
                vertices_count,
                indices_count,
                grx_aabb{to_vec3f(src->mAABB.mMin), to_vec3f(src->mAABB.mMax)}
            };
            vertices_count += src->mNumVertices;
            indices_count  += dst.indices_count;
            faces_count    += src->mNumFaces;
        }

        DLOG("Total vertices: {}", vertices_count);
        DLOG("Total indices:  {}", indices_count);
        DLOG("Total faces:    {}", faces_count);

        RASSERTF(faces_count * 3 == indices_count, "{}", "May be triangulate failed?");

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

        for (auto mesh : core::span(scene->mMeshes, scene->mNumMeshes)) {
            DLOG("Load mesh [{}]...", mesh->mName.C_Str());
            DLOG("Vertices count:       {}", mesh->mNumVertices);
            DLOG("Faces count:          {}", mesh->mNumFaces);
            DLOG("Has positions:        {}", mesh->HasPositions());
            DLOG("Has faces:            {}", mesh->HasFaces());
            DLOG("Has normals:          {}", mesh->HasNormals());
            DLOG("Has tan and bitan:    {}", mesh->HasTangentsAndBitangents());
            DLOG("Color channels count: {}", mesh->GetNumColorChannels());
            DLOG("UV channels count:    {}", mesh->GetNumUVChannels());
            DLOG("AABB: min: {.3} max: {.3}", to_vec3f(mesh->mAABB.mMin), to_vec3f(mesh->mAABB.mMax));

            if (!mesh->HasTangentsAndBitangents()) {
                LOG_WARNING("Mesh [{}] has not tangents, skipping", mesh->mName.C_Str());
                continue;
            }

            {
                RASSERTF(mesh->HasFaces(), "{}", "Mesh loader doesn't support meshes without faces");
                auto old_size = indices.size();
                indices.resize(old_size + mesh->mNumFaces * 3);
                for (auto& face : core::span(mesh->mFaces, mesh->mNumFaces)) {
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
                    for (auto& uv : core::span(mesh->mTextureCoords[0], mesh->mNumVertices))
                        uvs[old_size++] = core::vec{uv.x, uv.y};
                } else {
                    auto old_size = uvs.size();
                    uvs.resize(old_size + mesh->mNumVertices);
                    std::fill(uvs.begin() + static_cast<vbo_vector_vec2f::difference_type>(old_size),
                            uvs.end(), core::vec{0.0f, 0.0f});
                }
            }
        }

        mesh_vbo.template set_data<mesh_vbo_types::INDEX_BUF    >(indices);
        mesh_vbo.template set_data<mesh_vbo_types::POSITION_BUF >(positions);
        mesh_vbo.template set_data<mesh_vbo_types::UV_BUF       >(uvs);
//        mesh_vbo.template set_data<mesh_vbo_types::NORMAL_BUF   >(normals);
//        mesh_vbo.template set_data<mesh_vbo_types::TANGENT_BUF  >(tangents);
//        mesh_vbo.template set_data<mesh_vbo_types::BITANGENT_BUF>(bitangents);

        return { std::move(mesh_vbo), std::move(mesh_entries) };
    }
/*
    core::vector<grx_texture_set> load_texture_sets(
            const aiScene* scene,
            core::string_view dir,
            grx_texture_mgr& texture_mgr
    ) {
        core::vector<grx_texture_set> texture_set(scene->mNumMaterials);

        auto src_materials = core::span(scene->mMaterials, scene->mNumMaterials);
        for (auto& [dst_texture, material] : core::zip_view(texture_set, src_materials)) {
            DLOG("Load material [{}]", material->GetName().data);

            if (material->GetTextureCount(aiTextureType_DIFFUSE)) {
                aiString texture_path;
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path) == AI_SUCCESS) {
                    DLOG("\tLoad diffuse '{}'", texture_path.C_Str());
                    if (auto texture = texture_mgr.load(dir / core::string(texture_path.C_Str())))
                        dst_texture.diffuse() = *texture;
                    else
                        LOG_WARNING("Can't load texture '{}'", dir / core::string(texture_path.C_Str()));
                }
            }
            if (material->GetTextureCount(aiTextureType_NORMALS)) {
                aiString texture_path;
                if (material->GetTexture(aiTextureType_NORMALS, 0, &texture_path) == AI_SUCCESS) {
                    DLOG("\tLoad normal '{}'", texture_path.C_Str());
                    if (auto texture = texture_mgr.load(dir / core::string(texture_path.C_Str())))
                        dst_texture.normal() = *texture;
                    else
                        LOG_WARNING("Can't load texture '{}'", dir / core::string(texture_path.C_Str()));
                }
            }
            if (material->GetTextureCount(aiTextureType_SPECULAR)) {
                aiString texture_path;
                if (material->GetTexture(aiTextureType_SPECULAR, 0, &texture_path) == AI_SUCCESS) {
                    DLOG("\tLoad specular '{}'", texture_path.C_Str());
                    if (auto texture = texture_mgr.load(dir / core::string(texture_path.C_Str())))
                        dst_texture.specular() = *texture;
                    else
                        LOG_WARNING("Can't load texture '{}'", dir / core::string(texture_path.C_Str()));
                }
            }
        }

        return texture_set;
    }*/

    template <typename VboType>
    auto load_bones(
            const aiScene* scene,
            VboType& vbo,
            const core::vector<grx_mesh_entry>& mesh_entries,
            size_t vertices_count
    ) {
        vbo_vector_bone bones(vertices_count, vbo_vector_bone::value_type());

        core::hash_map<core::string, uint> bone_map;
        core::vector<grx_aabb>  aabbs;
        core::vector<glm::mat4> offsets;
        core::vector<glm::mat4> transforms;

        auto src_meshes = core::span(scene->mMeshes, scene->mNumMeshes);

        for (auto& [mesh, i] : core::value_index_view(src_meshes)) {
            for (auto bone : core::span(mesh->mBones, mesh->mNumBones)) {
                auto [position, was_inserted] = bone_map.emplace(bone->mName.data, offsets.size());

                if (was_inserted) {
                    DLOG("Load bone [{}]", bone->mName.data);

                    offsets   .emplace_back(to_glm(bone->mOffsetMatrix));
                    transforms.emplace_back(glm::mat4(1.f));
                    aabbs     .emplace_back(grx_aabb::maximized());
                }

                auto& aabb = aabbs[position->second];

                for (auto& weight : core::span(bone->mWeights, bone->mNumWeights)) {
                    auto vertex_id = mesh_entries[i].start_vertex_pos + weight.mVertexId;
                    bones[vertex_id].append(position->second, weight.mWeight);

                    auto vertex = to_vec3f(mesh->mVertices[weight.mVertexId]);
                    aabb.merge({vertex, vertex});
                }
            }
        }

        vbo.template set_data<mesh_vbo_skeleton_types::BONE_BUF>(bones);
        return core::tuple{std::move(bone_map), std::move(offsets), std::move(transforms), std::move(aabbs)};
    }

    /*
    void load_node_iter(bone_node* dst, aiNode* src, size_t level = 0) {
        DLOG("Node [{}]", src->mName.data);
        dst->name = src->mName.data;
        dst->transform = to_glm(src->mTransformation);

        dst->children.resize(src->mNumChildren);
        auto children = core::span(src->mChildren, src->mNumChildren);

        for (auto& [dst1, src1] : core::zip_view(dst->children, children)) {
            dst1 = core::make_unique<bone_node>();
            load_node_iter(dst1.get(), src1, level + 1);
        }
    }
    */

    void grx_mesh_mgr::load_anim(const aiScene* scene, grx_bone_data& bone_data) {
        bone_data.root_node = core::make_unique<bone_node>();
        load_node_iter(bone_data.root_node.get(), scene->mRootNode);

        DLOG("Animations count: {}", scene->mNumAnimations);

        for (auto anim : core::span(scene->mAnimations, scene->mNumAnimations)) {
            auto [pos, was_inserted] = bone_data.animations.emplace(core::string(anim->mName.data), grx_animation());

            DLOG("Animation name: {}", anim->mName.data);

            if (was_inserted) {
                pos->second.duration         = anim->mDuration;
                pos->second.ticks_per_second = anim->mTicksPerSecond;

                DLOG("Channels count: {}", anim->mNumChannels);

                for (auto channel : core::span(anim->mChannels, anim->mNumChannels)) {
                    DLOG("Channel name: {}", channel->mNodeName.data);

                    auto [pos2, was_inserted2] = pos->second.channels.emplace(
                            core::string(channel->mNodeName.data), anim_channel());

                    if (was_inserted2) {
                        auto& src_channel = pos2->second;

                        src_channel.position_keys.resize(channel->mNumPositionKeys);
                        src_channel.scaling_keys .resize(channel->mNumScalingKeys);
                        src_channel.rotation_keys.resize(channel->mNumRotationKeys);

                        auto src_position_keys = core::span(channel->mPositionKeys, channel->mNumPositionKeys);
                        auto src_scaling_keys  = core::span(channel->mScalingKeys, channel->mNumScalingKeys);
                        auto src_rotation_keys = core::span(channel->mRotationKeys, channel->mNumRotationKeys);

                        for (auto& [dst, src] : core::zip_view(src_channel.position_keys, src_position_keys)) {
                            dst.time  = src.mTime;
                            dst.value = core::vec{src.mValue.x, src.mValue.y, src.mValue.z};
                        }
                        for (auto& [dst, src] : core::zip_view(src_channel.scaling_keys, src_scaling_keys)) {
                            dst.time  = src.mTime;
                            dst.value = core::vec{src.mValue.x, src.mValue.y, src.mValue.z};
                        }
                        for (auto& [dst, src] : core::zip_view(src_channel.rotation_keys, src_rotation_keys)) {
                            dst.time  = src.mTime;
                            dst.value = glm::quat(src.mValue.w, src.mValue.x, src.mValue.y, src.mValue.z);
                        }
                    }
                }
            }
        }
    }

    bool is_scene_has_bones(const aiScene* scene) {
        return core::span(scene->mMeshes, scene->mNumMeshes) /
                   core::any_of([](auto& v) { return v->HasBones(); });
    }

    void grx_mesh_mgr::anim_traverse(
            grx_bone_data& bone_data,
            const grx_animation& anim,
            double time,
            const bone_node* node,
            const glm::mat4& parent_transform
    ) {
        auto get_neighborhood = [](double time, const auto& keys) {
            time = fmod(time, keys.back().time);
            auto next   = keys / core::find_if([=](auto& k) { return time < k.time; });
            auto cur    = next == keys.begin() ? keys.end() - 1 : next - 1;
            auto factor = static_cast<float>((time - cur->time) / (next->time - cur->time));
            return core::tuple{next, cur, factor};
        };

        auto calc_scaling = [=](double time, const anim_channel& c) -> glm::mat4 {
            if (c.scaling_keys.size() == 1)
                return glm::scale(glm::mat4(1.f), to_glm(c.scaling_keys.front().value));

            auto [next, cur, factor] = get_neighborhood(time, c.scaling_keys);
            return glm::scale(glm::mat4(1.f), to_glm(core::lerp(cur->value, next->value, factor)));
        };

        auto calc_rotation = [=](double time, const anim_channel& c) -> glm::mat4 {
            if (c.rotation_keys.size() == 1)
                return glm::mat4(c.rotation_keys.front().value);

            auto [next, cur, factor] = get_neighborhood(time, c.rotation_keys);
            return glm::mat4_cast(glm::normalize(glm::slerp(cur->value, next->value, factor)));
        };

        auto calc_position = [=](double time, const anim_channel& c) -> glm::mat4 {
            if (c.position_keys.size() == 1)
                return glm::translate(glm::mat4(1.f), to_glm(c.position_keys.front().value));

            auto [next, cur, factor] = get_neighborhood(time, c.position_keys);
            return glm::translate(glm::mat4(1.f), to_glm(core::lerp(cur->value, next->value, factor)));
        };

        auto channel_pos = anim.channels.find(node->name);
        glm::mat4 transform = node->transform;

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
} // namespace grx


auto grx::grx_mesh_mgr::
load_mesh(core::string_view path, bool instanced/*, grx_texture_mgr* texture_mgr*/) -> core::shared_ptr<grx_mesh>
{
    auto importer = Assimp::Importer();

    const aiScene* scene;
    auto flags = aiProcess_Triangulate |
                 aiProcess_GenBoundingBoxes |
                 aiProcess_GenSmoothNormals |
                 aiProcess_CalcTangentSpace |
                 aiProcess_FlipUVs |
                 aiProcess_JoinIdenticalVertices;

    if (path.ends_with(".dae")) {
        auto data = collada_prepare(path);
        scene = importer.ReadFileFromMemory(data.data(), data.length(), flags);
    } else {
        scene = importer.ReadFile(path.data(), flags);
    }

    RASSERTF(scene, "Can't load mesh at path '{}': {}", path, importer.GetErrorString());
    RASSERTF(scene->HasMeshes(), "Scene at path '{}' doesn't have any mesh", path);

    DLOG("Load scene data '{}'", path);

    core::shared_ptr<grx_mesh> mesh;

    if (instanced) {
        auto [mesh_vbo, mesh_entries] = load_mesh_vbo<mesh_instanced_vbo_t>(scene);
        mesh = core::shared_ptr<grx_mesh>(
                new grx_mesh(std::move(mesh_vbo), std::move(mesh_entries)));
        mesh->_instanced = true;
    }
    else if (is_scene_has_bones(scene)) {
        auto [mesh_vbo, mesh_entries]    = load_mesh_vbo<mesh_vbo_skeleton_t>(scene);
        auto [bone_map, bone_offsets, bone_transforms, bone_aabbs] =
                load_bones<mesh_vbo_skeleton_t>(scene, mesh_vbo, mesh_entries,
                mesh_entries.back().start_vertex_pos + mesh_entries.back().vertices_count * 3);

        mesh = core::shared_ptr<grx_mesh>(
                new grx_mesh(std::move(mesh_vbo), std::move(mesh_entries)));

        mesh->_bone_data = core::unique_ptr<grx_bone_data>(new grx_bone_data);
        mesh->_bone_data->bone_map         = std::move(bone_map);
        mesh->_bone_data->offsets          = std::move(bone_offsets);
        mesh->_bone_data->final_transforms = std::move(bone_transforms);
        mesh->_bone_data->aabbs            = std::move(bone_aabbs);
        mesh->_bone_data->global_inverse_transform = glm::inverse(to_glm(scene->mRootNode->mTransformation));

        load_anim(scene, *(mesh->_bone_data));
    }
    else {
        auto [mesh_vbo, mesh_entries] = load_mesh_vbo<mesh_vbo_t>(scene);
        mesh = core::shared_ptr<grx_mesh>(
                new grx_mesh(std::move(mesh_vbo), std::move(mesh_entries)));
    }

    //mesh->_texture_sets = load_texture_sets(scene, core::path_eval(path / ".."), *texture_mgr);

    // Calc AABB
    mesh->_aabb = mesh->_mesh_entries.front().aabb;
    for (auto& me : core::skip_view(mesh->_mesh_entries, 1))
        mesh->_aabb.merge(me.aabb);

    DLOG("AABB: min: {.3} max: {.3}", mesh->_aabb.min, mesh->_aabb.max);

    return mesh;
}

auto grx::grx_mesh_mgr::load(core::string_view p, bool instanced) -> core::shared_ptr<grx_mesh> {
    auto path = core::path_eval(p);

    std::lock_guard lock(_meshes_mutex);

    auto [position, was_inserted] = _meshes.emplace(path, nullptr);

    if (!was_inserted)
        return position->second;
    else
        return position->second = load_mesh(path, instanced/*, &_texture_mgr*/);
}

auto grx::grx_mesh_mgr::load(const core::config_manager& cm, core::string_view p, bool instanced)
-> core::shared_ptr<grx_mesh>
{
    return load(cm.entry_dir() / cm.read_unwrap<core::string>("models_dir") / p, instanced);
}

void grx::grx_mesh::draw(const glm::mat4& vp, const glm::mat4& model, shader_program_id_t program_id) {
    grx_shader_mgr::use_program(program_id);

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
        grx::grx_shader_mgr::set_uniform(_cached_uniform_mvp, vp * model);

    if (_cached_uniform_model != static_cast<uniform_id_t>(-1))
        grx::grx_shader_mgr::set_uniform(_cached_uniform_model, model);

    if (_bone_data) {
        glUniformMatrix4fv(
                static_cast<GLint>(_bone_data->cached_bone_matrices_uniform),
                static_cast<GLsizei>(_bone_data->final_transforms.size()),
                GL_FALSE,
                &(_bone_data->final_transforms.front()[0][0]));
    }

    _mesh_vbo.bind_vao();

    for (auto& entry : _mesh_entries) {
        //if (entry.material_index != std::numeric_limits<uint>::max())
            //_texture_sets[entry.material_index].bind(program_id);

        _mesh_vbo.draw(entry.indices_count, entry.start_vertex_pos, entry.start_index_pos);
    }
}

void grx::grx_mesh::
draw_instanced(const glm::mat4& vp, const core::vector<glm::mat4>& models, shader_program_id_t program_id) {
    auto& vbo_tuple = _mesh_vbo.cast<mesh_instanced_vbo_t>();

    auto mvps = core::vector<glm::mat4>(models.size());

    for (auto& [mvp, m] : core::zip_view(mvps, models))
        mvp = vp * m;

    vbo_tuple.set_data<mesh_vbo_types::MVP_MAT>  (mvps);
    vbo_tuple.set_data<mesh_vbo_types::MODEL_MAT>(models);

    vbo_tuple.bind_vao();

    for (auto& entry : _mesh_entries) {
        //if (entry.material_index != std::numeric_limits<uint>::max())
        //    _texture_sets[entry.material_index].bind(program_id);

        vbo_tuple.draw_instanced(
                models.size(),
                entry.indices_count,
                entry.start_vertex_pos,
                entry.start_index_pos);
    }
}

void grx::grx_mesh::
draw(const glm::mat4& vp, const glm::mat4& model, const grx_shader_tech& tech) {
    if (_bone_data) {
        //grx::grx_shader_mgr::use_program(tech.skeleton());
        draw(vp, model, tech.skeleton());
    }
    else {
        //grx::grx_shader_mgr::use_program(tech.base());
        draw(vp, model, tech.base());
    }
}

void grx::grx_mesh::
draw_instanced(const glm::mat4& vp, const core::vector<glm::mat4>& models, const grx_shader_tech& tech) {
    //grx::grx_shader_mgr::use_program(tech.instanced());
    draw_instanced(vp, models, tech.instanced());
}


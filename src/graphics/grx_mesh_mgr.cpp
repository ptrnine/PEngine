#include "grx_mesh_mgr.hpp"

#include <GL/glew.h>

#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <core/assert.hpp>
#include <core/config_manager.hpp>
#include <core/container_extensions.hpp>
#include <core/log.hpp>
#include <core/math.hpp>

#include "grx_camera.hpp"
#include "grx_debug.hpp"
#include "grx_shader_mgr.hpp"
#include "grx_shader_tech.hpp"
#include "grx_utils.hpp"

using namespace core;

namespace grx
{
inline glm::mat4 to_glm(const aiMatrix4x4& m) {
    glm::mat4 to;

    to[0][0] = m.a1; to[0][1] = m.b1; to[0][2] = m.c1; to[0][3] = m.d1;
    to[1][0] = m.a2; to[1][1] = m.b2; to[1][2] = m.c2; to[1][3] = m.d2;
    to[2][0] = m.a3; to[2][1] = m.b3; to[2][2] = m.c3; to[2][3] = m.d3;
    to[3][0] = m.a4; to[3][1] = m.b4; to[3][2] = m.c4; to[3][3] = m.d4;

    return to;
}

inline vec3f to_vec3f(const aiVector3D& v) {
    return vec3f{v.x, v.y, v.z};
}

mesh_data_basic mesh_extract_basic_data(const aiScene* scene) {
    DLOG("Meshes count:     {}", scene->mNumMeshes);
    DLOG("Textures count:   {}", scene->mNumTextures);
    DLOG("Materials count:  {}", scene->mNumMaterials);

    uint vertices_count = 0;
    uint indices_count  = 0;
    uint faces_count    = 0;

    vector<grx_mesh_entry> mesh_entries(scene->mNumMeshes);

    auto meshes = span(scene->mMeshes, scene->mNumMeshes);
    for (auto& [dst, src] : zip_view(mesh_entries, meshes)) {
        dst = grx_mesh_entry{src->mNumFaces * 3, // Indices count
                             src->mNumVertices,
                             src->mMaterialIndex,
                             vertices_count,
                             indices_count,
                             grx_aabb{to_vec3f(src->mAABB.mMin), to_vec3f(src->mAABB.mMax)}};

        vertices_count += src->mNumVertices;
        indices_count += dst.indices_count;
        faces_count += src->mNumFaces;
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
        memcpy(vec.data() + old_size, data, sizeof(vec3f) * count);
    };

    for (auto mesh : span(scene->mMeshes, scene->mNumMeshes)) {
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

            for (auto& face : span(mesh->mFaces, mesh->mNumFaces)) {
                ASSERTF(face.mNumIndices == 3, "{}", "Mesh loader support 3 indices per face only");
                indices[old_size++] = face.mIndices[0];
                indices[old_size++] = face.mIndices[1];
                indices[old_size++] = face.mIndices[2];
            }
        }

        vector_vec3f_append(positions, mesh->mVertices, mesh->mNumVertices);
        vector_vec3f_append(normals, mesh->mNormals, mesh->mNumVertices);
        vector_vec3f_append(tangents, mesh->mTangents, mesh->mNumVertices);
        vector_vec3f_append(bitangents, mesh->mBitangents, mesh->mNumVertices);

        {
            if (mesh->GetNumUVChannels() > 0 && mesh->HasTextureCoords(0)) {
                auto old_size = uvs.size();
                uvs.resize(old_size + mesh->mNumVertices);
                for (auto& uv : span(mesh->mTextureCoords[0], mesh->mNumVertices)) uvs[old_size++] = vec{uv.x, uv.y};
            }
            else {
                auto old_size = uvs.size();
                uvs.resize(old_size + mesh->mNumVertices);
                std::fill(
                    uvs.begin() + static_cast<vbo_vector_vec2f::difference_type>(old_size), uvs.end(), vec{0.0f, 0.0f});
            }
        }
    }

    return {
        move(mesh_entries), move(indices), move(positions), move(uvs), move(normals), move(tangents), move(bitangents)};
}

void load_node_iter(bone_node* dst, aiNode* src) {
    dst->name      = src->mName.data;
    dst->transform = to_glm(src->mTransformation);

    dst->children.resize(src->mNumChildren);
    auto children = span(src->mChildren, src->mNumChildren);

    for (auto& [dst1, src1] : zip_view(dst->children, children)) {
        dst1 = make_unique<bone_node>();
        load_node_iter(dst1.get(), src1);
    }
}

mesh_bone_data mesh_extract_bone_data(const aiScene* scene, const vector<grx_mesh_entry>& mesh_entries) {
    auto skeleton_tree = make_unique<bone_node>();
    load_node_iter(skeleton_tree.get(), scene->mRootNode);

    auto            vertices_count = mesh_entries.back().start_vertex_pos + mesh_entries.back().vertices_count;
    vbo_vector_bone bones(vertices_count, vbo_vector_bone::value_type());

    hash_map<string, uint> bone_map;
    vector<grx_aabb>       aabbs;
    vector<glm::mat4>      offsets;
    vector<glm::mat4>      transforms;

    auto src_meshes = span(scene->mMeshes, scene->mNumMeshes);

    for (auto& [mesh, i] : value_index_view(src_meshes)) {
        for (auto bone : span(mesh->mBones, mesh->mNumBones)) {
            auto [position, was_inserted] = bone_map.emplace(bone->mName.data, offsets.size());

            if (was_inserted) {
                DLOG("Load bone [{}]", bone->mName.data);

                offsets.emplace_back(to_glm(bone->mOffsetMatrix));
                transforms.emplace_back(glm::mat4(1.f));
                aabbs.emplace_back(grx_aabb::maximized());
            }

            auto& aabb = aabbs[position->second];

            for (auto& weight : span(bone->mWeights, bone->mNumWeights)) {
                auto vertex_id = mesh_entries[i].start_vertex_pos + weight.mVertexId;
                bones[vertex_id].append(position->second, weight.mWeight);

                auto vertex = to_vec3f(mesh->mVertices[weight.mVertexId]);
                aabb.merge({vertex, vertex});
            }
        }
    }

    return {move(bones), move(skeleton_tree), move(bone_map), move(aabbs), move(offsets), move(transforms)};
}

hash_map<string, grx_animation> mesh_extract_animations(const aiScene* scene) {
    hash_map<string, grx_animation> animations;

    DLOG("Animations count: {}", scene->mNumAnimations);

    for (auto anim : span(scene->mAnimations, scene->mNumAnimations)) {
        auto [pos, was_inserted] = animations.emplace(string(anim->mName.data), grx_animation());

        DLOG("Animation name: {}", anim->mName.data);

        if (was_inserted) {
            pos->second.duration         = anim->mDuration;
            pos->second.ticks_per_second = anim->mTicksPerSecond;

            DLOG("Channels count: {}", anim->mNumChannels);

            for (auto channel : span(anim->mChannels, anim->mNumChannels)) {
                DLOG("Channel name: {}", channel->mNodeName.data);

                auto [pos2, was_inserted2] =
                    pos->second.channels.emplace(string(channel->mNodeName.data), anim_channel());

                if (was_inserted2) {
                    auto& src_channel = pos2->second;

                    src_channel.position_keys.resize(channel->mNumPositionKeys);
                    src_channel.scaling_keys.resize(channel->mNumScalingKeys);
                    src_channel.rotation_keys.resize(channel->mNumRotationKeys);

                    auto src_position_keys = span(channel->mPositionKeys, channel->mNumPositionKeys);
                    auto src_scaling_keys  = span(channel->mScalingKeys, channel->mNumScalingKeys);
                    auto src_rotation_keys = span(channel->mRotationKeys, channel->mNumRotationKeys);

                    for (auto& [dst, src] : zip_view(src_channel.position_keys, src_position_keys)) {
                        dst.time  = src.mTime;
                        dst.value = vec{src.mValue.x, src.mValue.y, src.mValue.z};
                    }
                    for (auto& [dst, src] : zip_view(src_channel.scaling_keys, src_scaling_keys)) {
                        dst.time  = src.mTime;
                        dst.value = vec{src.mValue.x, src.mValue.y, src.mValue.z};
                    }
                    for (auto& [dst, src] : zip_view(src_channel.rotation_keys, src_rotation_keys)) {
                        dst.time  = src.mTime;
                        dst.value = glm::quat(src.mValue.w, src.mValue.x, src.mValue.y, src.mValue.z);
                    }
                }
            }
        }
    }

    return animations;
}

vector<texture_path_pack> mesh_extract_texture_paths(const aiScene* scene, const string& dir) {
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
pair<VboT, vector<grx_mesh_entry>> load_mesh_vbo(const aiScene* scene) {
    VboT mesh_vbo;

    DLOG("Meshes count:     {}", scene->mNumMeshes);
    DLOG("Textures count:   {}", scene->mNumTextures);
    DLOG("Materials count:  {}", scene->mNumMaterials);

    uint vertices_count = 0;
    uint indices_count  = 0;
    uint faces_count    = 0;

    vector<grx_mesh_entry> mesh_entries(scene->mNumMeshes);
    auto                   meshes = span(scene->mMeshes, scene->mNumMeshes);
    for (auto& [dst, src] : zip_view(mesh_entries, meshes)) {
        dst = grx_mesh_entry{src->mNumFaces * 3, // Indices count
                             src->mNumVertices,
                             src->mMaterialIndex,
                             vertices_count,
                             indices_count,
                             grx_aabb{to_vec3f(src->mAABB.mMin), to_vec3f(src->mAABB.mMax)}};

        vertices_count += src->mNumVertices;
        indices_count += dst.indices_count;
        faces_count += src->mNumFaces;
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
        memcpy(vec.data() + old_size, data, sizeof(vec3f) * count);
    };

    for (auto mesh : span(scene->mMeshes, scene->mNumMeshes)) {
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
            for (auto& face : span(mesh->mFaces, mesh->mNumFaces)) {
                ASSERTF(face.mNumIndices == 3, "{}", "Mesh loader support 3 indices per face only");
                indices[old_size++] = face.mIndices[0];
                indices[old_size++] = face.mIndices[1];
                indices[old_size++] = face.mIndices[2];
            }
        }

        vector_vec3f_append(positions, mesh->mVertices, mesh->mNumVertices);
        vector_vec3f_append(normals, mesh->mNormals, mesh->mNumVertices);
        vector_vec3f_append(tangents, mesh->mTangents, mesh->mNumVertices);
        vector_vec3f_append(bitangents, mesh->mBitangents, mesh->mNumVertices);

        {
            if (mesh->GetNumUVChannels() > 0 && mesh->HasTextureCoords(0)) {
                auto old_size = uvs.size();
                uvs.resize(old_size + mesh->mNumVertices);
                for (auto& uv : span(mesh->mTextureCoords[0], mesh->mNumVertices)) uvs[old_size++] = vec{uv.x, uv.y};
            }
            else {
                auto old_size = uvs.size();
                uvs.resize(old_size + mesh->mNumVertices);
                std::fill(
                    uvs.begin() + static_cast<vbo_vector_vec2f::difference_type>(old_size), uvs.end(), vec{0.0f, 0.0f});
            }
        }
    }

    mesh_vbo.template set_data<mesh_vbo_types::INDEX_BUF>(indices);
    mesh_vbo.template set_data<mesh_vbo_types::POSITION_BUF>(positions);
    mesh_vbo.template set_data<mesh_vbo_types::UV_BUF>(uvs);
    //        mesh_vbo.template set_data<mesh_vbo_types::NORMAL_BUF   >(normals);
    //        mesh_vbo.template set_data<mesh_vbo_types::TANGENT_BUF  >(tangents);
    //        mesh_vbo.template set_data<mesh_vbo_types::BITANGENT_BUF>(bitangents);

    return {std::move(mesh_vbo), std::move(mesh_entries)};
}

vector<grx_texture_set<4>>
mesh_load_texture_sets(const aiScene* scene, const string& dir, grx_texture_mgr& texture_mgr) {
    auto                       texsets_paths = mesh_extract_texture_paths(scene, dir);
    vector<grx_texture_set<4>> texsets(texsets_paths.size());

    auto load_if_valid = [&texture_mgr](auto& texture_set, const auto& path_opt, size_t index) {
        if (path_opt)
            texture_set.set(index, texture_mgr.load_async<color_rgba>(*path_opt));
        else
            texture_set.set(index);
    };

    for (auto& [texset, texset_paths] : core::zip_view(texsets, texsets_paths)) {
        load_if_valid(texset, texset_paths.diffuse, grx_texture_set<4>::diffuse);
        load_if_valid(texset, texset_paths.normal, grx_texture_set<4>::normal);
        load_if_valid(texset, texset_paths.specular, grx_texture_set<4>::specular);
    }

    return texsets;
}

template <typename VboType>
auto load_bones(const aiScene* scene, VboType& vbo, const vector<grx_mesh_entry>& mesh_entries, size_t vertices_count) {
    vbo_vector_bone bones(vertices_count, vbo_vector_bone::value_type());

    hash_map<string, uint> bone_map;
    vector<grx_aabb>       aabbs;
    vector<glm::mat4>      offsets;
    vector<glm::mat4>      transforms;

    auto src_meshes = span(scene->mMeshes, scene->mNumMeshes);

    for (auto& [mesh, i] : value_index_view(src_meshes)) {
        for (auto bone : span(mesh->mBones, mesh->mNumBones)) {
            auto [position, was_inserted] = bone_map.emplace(bone->mName.data, offsets.size());

            if (was_inserted) {
                DLOG("Load bone [{}]", bone->mName.data);

                offsets.emplace_back(to_glm(bone->mOffsetMatrix));
                transforms.emplace_back(glm::mat4(1.f));
                aabbs.emplace_back(grx_aabb::maximized());
            }

            auto& aabb = aabbs[position->second];

            for (auto& weight : span(bone->mWeights, bone->mNumWeights)) {
                auto vertex_id = mesh_entries[i].start_vertex_pos + weight.mVertexId;
                bones[vertex_id].append(position->second, weight.mWeight);

                auto vertex = to_vec3f(mesh->mVertices[weight.mVertexId]);
                aabb.merge({vertex, vertex});
            }
        }
    }

    vbo.template set_data<mesh_vbo_skeleton_types::BONE_BUF>(bones);
    return tuple{std::move(bone_map), std::move(offsets), std::move(transforms), std::move(aabbs)};
}

void grx_mesh_mgr::load_anim(const aiScene* scene, grx_bone_data& bone_data) {
    bone_data.root_node = make_unique<bone_node>();
    load_node_iter(bone_data.root_node.get(), scene->mRootNode);

    DLOG("Animations count: {}", scene->mNumAnimations);

    for (auto anim : span(scene->mAnimations, scene->mNumAnimations)) {
        auto [pos, was_inserted] = bone_data.animations.emplace(string(anim->mName.data), grx_animation());

        DLOG("Animation name: {}", anim->mName.data);

        if (was_inserted) {
            pos->second.duration         = anim->mDuration;
            pos->second.ticks_per_second = anim->mTicksPerSecond;

            DLOG("Channels count: {}", anim->mNumChannels);

            for (auto channel : span(anim->mChannels, anim->mNumChannels)) {
                DLOG("Channel name: {}", channel->mNodeName.data);

                auto [pos2, was_inserted2] =
                    pos->second.channels.emplace(string(channel->mNodeName.data), anim_channel());

                if (was_inserted2) {
                    auto& src_channel = pos2->second;

                    src_channel.position_keys.resize(channel->mNumPositionKeys);
                    src_channel.scaling_keys.resize(channel->mNumScalingKeys);
                    src_channel.rotation_keys.resize(channel->mNumRotationKeys);

                    auto src_position_keys = span(channel->mPositionKeys, channel->mNumPositionKeys);
                    auto src_scaling_keys  = span(channel->mScalingKeys, channel->mNumScalingKeys);
                    auto src_rotation_keys = span(channel->mRotationKeys, channel->mNumRotationKeys);

                    for (auto& [dst, src] : zip_view(src_channel.position_keys, src_position_keys)) {
                        dst.time  = src.mTime;
                        dst.value = vec{src.mValue.x, src.mValue.y, src.mValue.z};
                    }
                    for (auto& [dst, src] : zip_view(src_channel.scaling_keys, src_scaling_keys)) {
                        dst.time  = src.mTime;
                        dst.value = vec{src.mValue.x, src.mValue.y, src.mValue.z};
                    }
                    for (auto& [dst, src] : zip_view(src_channel.rotation_keys, src_rotation_keys)) {
                        dst.time  = src.mTime;
                        dst.value = glm::quat(src.mValue.w, src.mValue.x, src.mValue.y, src.mValue.z);
                    }
                }
            }
        }
    }
}

bool is_scene_has_bones(const aiScene* scene) {
    return span(scene->mMeshes, scene->mNumMeshes) / any_of([](auto& v) { return v->HasBones(); });
}

void grx_mesh_mgr::anim_traverse(grx_bone_data&       bone_data,
                                 const grx_animation& anim,
                                 double               time,
                                 const bone_node*     node,
                                 const glm::mat4&     parent_transform) {
    auto get_neighborhood = [](double time, const auto& keys) {
        time        = fmod(time, keys.back().time);
        auto next   = keys / find_if([=](auto& k) { return time < k.time; });
        auto cur    = next == keys.begin() ? keys.end() - 1 : next - 1;
        auto factor = static_cast<float>((time - cur->time) / (next->time - cur->time));
        return tuple{next, cur, factor};
    };

    auto calc_scaling = [=](double time, const anim_channel& c) -> glm::mat4 {
        if (c.scaling_keys.size() == 1)
            return glm::scale(glm::mat4(1.f), to_glm(c.scaling_keys.front().value));

        auto [next, cur, factor] = get_neighborhood(time, c.scaling_keys);
        return glm::scale(glm::mat4(1.f), to_glm(lerp(cur->value, next->value, factor)));
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
        return glm::translate(glm::mat4(1.f), to_glm(lerp(cur->value, next->value, factor)));
    };

    auto      channel_pos = anim.channels.find(node->name);
    glm::mat4 transform   = node->transform;

    if (channel_pos != anim.channels.end()) {
        auto scaling     = calc_scaling(time, channel_pos->second);
        auto rotation    = calc_rotation(time, channel_pos->second);
        auto translation = calc_position(time, channel_pos->second);

        transform = translation * scaling * rotation;
    }

    glm::mat4 global_transform  = parent_transform * transform;
    auto      transform_idx_pos = bone_data.bone_map.find(node->name);

    if (transform_idx_pos != bone_data.bone_map.end()) {
        bone_data.final_transforms[transform_idx_pos->second] =
            bone_data.global_inverse_transform * global_transform * bone_data.offsets[transform_idx_pos->second];
    }

    for (auto& child : node->children)
        anim_traverse(bone_data, anim, time, child.get(), global_transform);
}

string collada_prepare(string_view path) {
    std::ifstream ifs(string(path), std::ios::in | std::ios::binary);

    if (!ifs.is_open())
        return "";

    ifs.seekg(0, std::ios::end);
    auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    auto size = static_cast<size_t>(end - ifs.tellg());

    string data;
    data.resize(size);
    ifs.read(data.data(), static_cast<std::streamsize>(size));

    return grx_utils::collada_bake_bind_shape_matrix(data);
}
} // namespace grx

grx::grx_mesh_mgr::grx_mesh_mgr(const core::config_manager& cm) {
    _texture_mgr = grx_texture_mgr::create_shared(cm);
    _models_dir  = cm.entry_dir() / cm.read_unwrap<string>("models_dir");
}

auto grx::grx_mesh_mgr::load_mesh(string_view path, bool instanced, grx_texture_mgr* texture_mgr)
    -> shared_ptr<grx_mesh> {
    auto importer = Assimp::Importer();

    const aiScene* scene; // NOLINT
    auto flags = aiProcess_Triangulate            |
                 aiProcess_GenBoundingBoxes       |
                 aiProcess_GenSmoothNormals       |
                 aiProcess_CalcTangentSpace       |
                 aiProcess_FlipUVs                |
                 aiProcess_JoinIdenticalVertices;

    if (path.ends_with(".dae")) {
        auto data = collada_prepare(path);
        scene     = importer.ReadFileFromMemory(data.data(), data.length(), flags);
    }
    else {
        scene = importer.ReadFile(path.data(), flags);
    }

    RASSERTF(scene, "Can't load mesh at path '{}': {}", path, importer.GetErrorString());
    RASSERTF(scene->HasMeshes(), "Scene at path '{}' doesn't have any mesh", path);

    DLOG("Load scene data '{}'", path);

    shared_ptr<grx_mesh> mesh;

    if (instanced) {
        auto [mesh_vbo, mesh_entries] = load_mesh_vbo<mesh_instanced_vbo_t>(scene);
        mesh             = shared_ptr<grx_mesh>(new grx_mesh(std::move(mesh_vbo), std::move(mesh_entries)));
        mesh->_instanced = true;
    }
    else if (is_scene_has_bones(scene)) {
        auto [mesh_vbo, mesh_entries]                              = load_mesh_vbo<mesh_vbo_skeleton_t>(scene);
        auto [bone_map, bone_offsets, bone_transforms, bone_aabbs] = load_bones<mesh_vbo_skeleton_t>(
            scene,
            mesh_vbo,
            mesh_entries,
            mesh_entries.back().start_vertex_pos + mesh_entries.back().vertices_count * 3);

        mesh = shared_ptr<grx_mesh>(new grx_mesh(std::move(mesh_vbo), std::move(mesh_entries)));

        mesh->_bone_data                           = make_unique<grx_bone_data>();
        mesh->_bone_data->bone_map                 = std::move(bone_map);
        mesh->_bone_data->offsets                  = std::move(bone_offsets);
        mesh->_bone_data->final_transforms         = std::move(bone_transforms);
        mesh->_bone_data->aabbs                    = std::move(bone_aabbs);
        mesh->_bone_data->global_inverse_transform = glm::inverse(to_glm(scene->mRootNode->mTransformation));

        load_anim(scene, *(mesh->_bone_data));
    }
    else {
        auto [mesh_vbo, mesh_entries] = load_mesh_vbo<mesh_vbo_t>(scene);
        mesh = shared_ptr<grx_mesh>(new grx_mesh(std::move(mesh_vbo), std::move(mesh_entries)));
    }

    mesh->_texture_sets = mesh_load_texture_sets(scene, path_eval(path / ".."), *texture_mgr);

    // Calc AABB
    mesh->_aabb = mesh->_mesh_entries.front().aabb;
    for (auto& me : skip_view(mesh->_mesh_entries, 1)) mesh->_aabb.merge(me.aabb);

    return mesh;
}

auto grx::grx_mesh_mgr::load(string_view p, bool instanced) -> shared_ptr<grx_mesh> {
    auto path = path_eval(_models_dir / p);

    std::lock_guard lock(_meshes_mutex);

    auto [position, was_inserted] = _meshes.emplace(path, nullptr);

    if (!was_inserted)
        return position->second;
    else
        return position->second = load_mesh(path, instanced, _texture_mgr.get());
}

void grx::grx_mesh::draw(const glm::mat4& vp, const glm::mat4& model, const shared_ptr<grx_shader_program>& program) {
    program->activate();

    if (_cached_program != program.get() || !_cached_program) {
        _cached_program = program.get();
        _cached_uniform_mvp.emplace(_cached_program->get_uniform_unwrap<glm::mat4>("MVP"));

        if (_bone_data)
            _bone_data->cached_bone_matrices_uniform.emplace(
                _cached_program->get_uniform_unwrap<core::span<glm::mat4>>("bone_matrices"));
    }

    if (_cached_uniform_mvp)
        *_cached_uniform_mvp = vp * model;

    if (_bone_data && _bone_data->cached_bone_matrices_uniform)
        *_bone_data->cached_bone_matrices_uniform = core::span<glm::mat4>(_bone_data->final_transforms);

    _mesh_vbo.bind_vao();

    for (auto& entry : _mesh_entries) {
        if (entry.material_index != std::numeric_limits<uint>::max()) {
            auto& texture_resource = _texture_sets[entry.material_index].get_resource(0); // 0 is diffuse
            if (texture_resource.is_ready())
                texture_resource.get().value().bind_unit(0);
        }

        _mesh_vbo.draw(entry.indices_count, entry.start_vertex_pos, entry.start_index_pos);
    }
}

void grx::grx_mesh::draw_instanced(const glm::mat4&                      vp,
                                   const vector<glm::mat4>&              models,
                                   const shared_ptr<grx_shader_program>& program) {
    program->activate();

    auto& vbo_tuple = _mesh_vbo.cast<mesh_instanced_vbo_t>();

    auto mvps = vector<glm::mat4>(models.size());

    for (auto& [mvp, m] : zip_view(mvps, models)) mvp = vp * m;

    vbo_tuple.set_data<mesh_vbo_types::MVP_MAT>(mvps);
    vbo_tuple.set_data<mesh_vbo_types::MODEL_MAT>(models);

    vbo_tuple.bind_vao();

    for (auto& entry : _mesh_entries) {
        if (entry.material_index != std::numeric_limits<uint>::max()) {
            auto& texture_resource = _texture_sets[entry.material_index].get_resource(0); // 0 is diffuse
            if (texture_resource.is_ready())
                texture_resource.get().value().bind_unit(0);
        }

        if (entry.material_index != std::numeric_limits<uint>::max()) {
            auto& texture_resource = _texture_sets[entry.material_index].get_resource(0); // 0 is diffuse
            if (texture_resource.is_ready())
                texture_resource.get().value().bind_unit(0);
        }

        vbo_tuple.draw_instanced(models.size(), entry.indices_count, entry.start_vertex_pos, entry.start_index_pos);
    }
}

void grx::grx_mesh::draw(const glm::mat4& vp, const glm::mat4& model, const grx_shader_tech& tech) {
    if (_bone_data)
        draw(vp, model, tech.skeleton());
    else
        draw(vp, model, tech.base());
}

void grx::grx_mesh::draw_instanced(const glm::mat4& vp, const vector<glm::mat4>& models, const grx_shader_tech& tech) {
    draw_instanced(vp, models, tech.instanced());
}


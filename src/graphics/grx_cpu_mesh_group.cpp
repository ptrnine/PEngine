#include "grx_cpu_mesh_group.hpp"

#include <core/string_hash.hpp>
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

using namespace core;

namespace grx::details
{
span<int>::size_type span_size_cast(uint index) {
    return static_cast<span<int>::size_type>(index);
}

span<aiMesh*> assimp_get_meshes(const aiScene* scene) {
    return span(scene->mMeshes, scene->mNumMeshes);
}

uint assimp_vertices_count(const aiMesh* m) {
    if (!m->HasFaces())
        throw std::runtime_error("Mesh has no faces!");

    return m->mNumFaces * 3;
}

uint assimp_indices_count(const aiMesh* m) {
    if (m->mNumVertices == 0)
        throw std::runtime_error("Mesh has no vertices");

    return m->mNumVertices;
}

uint assimp_load_indices(const aiMesh* m, vector<u32>& buff, uint start_index) {
    if (!m->HasFaces())
        throw std::runtime_error("Mesh has no faces!");

    buff.resize(start_index + m->mNumFaces * 3);

    for (uint i = start_index; auto& face : span(m->mFaces, m->mNumFaces)) {
        buff[i + 0] = face.mIndices[0];
        buff[i + 1] = face.mIndices[1];
        buff[i + 2] = face.mIndices[2];
        i += 3;
    }

    return m->mNumFaces * 3;
}

uint assimp_load_positions(const aiMesh* m, vector<vec3f>& buff, uint start_index) {
    static_assert(sizeof(vec3f) == sizeof(m->mVertices[0]));

    if (m->mNumVertices == 0)
        throw std::runtime_error("Mesh has no vertices");

    buff.resize(start_index + m->mNumVertices);

    memcpy(buff.data() + start_index, m->mVertices, sizeof(vec3f) * m->mNumVertices);

    return m->mNumVertices;
}

uint assimp_load_uvs(const aiMesh* m, vector<vec2f>& buff, uint start_index) {
    if (m->GetNumUVChannels() < 1)
        throw std::runtime_error("Mesh has no uv channels");
    if (m->GetNumUVChannels() > 1)
        throw std::runtime_error("Mesh has more than one uv channel");
    if (!m->HasTextureCoords(0))
        throw std::runtime_error("Mesh has no uv coords");

    buff.resize(start_index + m->mNumVertices);
    auto buf = span<vec2f>(buff).subspan(span_size_cast(start_index));
    auto uvs = span(m->mTextureCoords[0], m->mNumVertices);

    for (auto& [dst, src] : zip_view(buf, uvs))
        dst.xy(src.x, src.y);

    return m->mNumVertices;
}

uint assimp_load_normals(const aiMesh* m, vector<vec3f>& buff, uint start_index) {
    static_assert(sizeof(vec3f) == sizeof(m->mNormals[0]));

    if (!m->HasNormals())
        throw std::runtime_error("Mesh has no normals");

    buff.resize(start_index + m->mNumVertices);

    memcpy(buff.data() + start_index, m->mNormals, sizeof(vec3f) * m->mNumVertices);

    return m->mNumVertices;
}

uint assimp_load_tangents(const aiMesh* m, vector<vec3f>& buff, uint start_index) {
    static_assert(sizeof(vec3f) == sizeof(m->mTangents[0]));

    if (!m->HasTangentsAndBitangents())
        throw std::runtime_error("Mesh has no tangents");

    buff.resize(start_index + m->mNumVertices);

    memcpy(buff.data() + start_index, m->mTangents, sizeof(vec3f) * m->mNumVertices);

    return m->mNumVertices;
}

uint assimp_load_bitangents(const aiMesh* m, vector<vec3f>& buff, uint start_index) {
    static_assert(sizeof(vec3f) == sizeof(m->mBitangents[0]));

    if (!m->HasTangentsAndBitangents())
        throw std::runtime_error("Mesh has no bitangents");

    buff.resize(start_index + m->mNumVertices);

    memcpy(buff.data() + start_index, m->mBitangents, sizeof(vec3f) * m->mNumVertices);

    return m->mNumVertices;
}

uint assimp_load_bones(const aiMesh* m, vector<grx_bone_vertex_data>& buff, uint start_index) {
    for (auto bone : span(m->mBones, m->mNumBones)) {
        for (auto& weight : span(bone->mWeights, bone->mNumWeights)) {
            auto vertex_id = start_index + weight.mVertexId;
            if (vertex_id >= buff.size())
                buff.resize(vertex_id + 1);
            buff[vertex_id].append(hash_fnv1a32(string_view(bone->mName.data, bone->mName.length)),
                                   weight.mWeight);
        }
    }
    return m->mNumVertices;
}

grx_aabb assimp_get_aabb(const aiMesh* m) {
    constexpr auto to_glm = [](const aiVector3D& v) {
        return vec3f{v.x, v.y, v.z};
    };
    return grx_aabb{to_glm(m->mAABB.mMin), to_glm(m->mAABB.mMax)};
}

grx_skeleton_data assimp_load_skeleton_data_map(const aiScene* scene) {
    if (!scene->HasMeshes())
        throw std::runtime_error("Mesh does not have meshes");

    grx_skeleton_data result;
    auto& mapping = result.mapping;

    auto src_meshes = span{scene->mMeshes, scene->mNumMeshes};
    for (auto& [mesh, i] : value_index_view(src_meshes))
        for (auto bone : span{mesh->mBones, mesh->mNumBones})
            mapping.emplace(bone->mName.data, mapping.size());

    return result;
}

uint assimp_get_material_index(const aiMesh* m) {
    return m->mMaterialIndex;
}

const aiScene* assimp_load_scene(string_view data) {
    auto flags = aiProcess_Triangulate            |
                 aiProcess_GenBoundingBoxes       |
                 aiProcess_GenSmoothNormals       |
                 aiProcess_CalcTangentSpace       |
                 aiProcess_FlipUVs                |
                 aiProcess_JoinIdenticalVertices;

    const aiScene* scene = aiImportFileFromMemory(data.data(), // NOLINT
                                                  static_cast<unsigned int>(data.size()),
                                                  flags,
                                                  "");
    return scene;
}

string assimp_get_error() {
    return aiGetErrorString();
}

void assimp_release_scene(const aiScene* scene) {
    aiReleaseImport(scene);
}


} // namespace grx::details

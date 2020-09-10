#pragma once

#include "grx_bone_data.hpp"
#include "grx_shader.hpp"
#include "grx_texture_set.hpp"
#include "grx_types.hpp"
#include "grx_vbo_tuple.hpp"

namespace grx
{
using core::hash_map;
using core::optional;
using core::shared_ptr;
using core::string;
using core::tuple;
using core::unique_ptr;
using core::vector;

using mesh_data_basic = tuple<vector<struct grx_mesh_entry>,
                              vbo_vector_indices, // Index buffer
                              vbo_vector_vec3f,   // Position buffer
                              vbo_vector_vec2f,   // UV buffer
                              vbo_vector_vec3f,   // Normal buffer
                              vbo_vector_vec3f,   // Tangent buffer
                              vbo_vector_vec3f    // Bitangent buffer
                              >;

using mesh_bone_data = tuple<vbo_vector_bone,              // Bone buffer
                             unique_ptr<struct bone_node>, // Skeleton tree
                             hash_map<string, uint>,       // Name - bone id map
                             vector<grx_aabb>,             // Aabbs
                             vector<glm::mat4>,            // Offsets matrices
                             vector<glm::mat4>             // Final transform matrices
                             >;

struct texture_path_pack {
    optional<string> diffuse;
    optional<string> normal;
    optional<string> specular;
};

/**
 * Simple mesh (no skeleton)
 */
using mesh_vbo_t = grx_vbo_tuple<vbo_vector_indices, // Index buffer
                                 vbo_vector_vec3f,   // Position buffer
                                 vbo_vector_vec2f,   // UV buffer
                                 vbo_vector_vec3f,   // Normal buffer
                                 vbo_vector_vec3f,   // Tangent buffer
                                 vbo_vector_vec3f    // Bitangent buffer
                                 >;

using mesh_buf_t = tuple<vbo_vector_indices, // Index buffer
                         vbo_vector_vec3f,   // Position buffer
                         vbo_vector_vec2f,   // UV buffer
                         vbo_vector_vec3f,   // Normal buffer
                         vbo_vector_vec3f,   // Tangent buffer
                         vbo_vector_vec3f    // Bitangent buffer
                         >;

/**
 * @brief Simple mesh, instanced rendering (no skeleton)
 */
using mesh_instanced_vbo_t = grx_vbo_tuple<vbo_vector_indices, // Index buffer
                                           vbo_vector_vec3f,   // Position buffer
                                           vbo_vector_vec2f,   // UV buffer
                                           vbo_vector_vec3f,   // Normal buffer
                                           vbo_vector_vec3f,   // Tangent buffer
                                           vbo_vector_vec3f,   // Bitangent buffer
                                           vbo_vector_matrix4, // ModelViewProjection matrix
                                           vbo_vector_matrix4  // Model matrix
                                           >;

/**
 * Mesh with skeleton
 */
using mesh_vbo_skeleton_t = grx_vbo_tuple<vbo_vector_indices, // Index buffer
                                          vbo_vector_vec3f,   // Position buffer
                                          vbo_vector_vec2f,   // UV buffer
                                          vbo_vector_vec3f,   // Normal buffer
                                          vbo_vector_vec3f,   // Tangent buffer
                                          vbo_vector_vec3f,   // Bitangent buffer
                                          vbo_vector_bone>;

/**
 * @brief Types of mesh_vbo_t components
 */
namespace mesh_vbo_types
{
    enum mesh_vbo_types {
        INDEX_BUF = 0,
        POSITION_BUF,
        UV_BUF,
        NORMAL_BUF,
        TANGENT_BUF,
        BITANGENT_BUF,
        MVP_MAT,
        MODEL_MAT
    };
}

/**
 * @brief Types of mesh_vbo_skeleton_t components
 */
namespace mesh_vbo_skeleton_types
{
    enum mesh_vbo_skeleton_types {
        INDEX_BUF = 0,
        POSITION_BUF,
        UV_BUF,
        NORMAL_BUF,
        TANGENT_BUF,
        BITANGENT_BUF,
        BONE_BUF
    };
}

/**
 * @brief Stores data of one mesh group
 */
struct grx_mesh_entry { // NOLINT
    uint     indices_count;
    uint     vertices_count;
    uint     material_index = std::numeric_limits<uint>::max();
    uint     start_vertex_pos;
    uint     start_index_pos;
    grx_aabb aabb;
};


/**
 * @brief Represents a mesh
 */
class grx_mesh {
public:
    /**
     * @brief Renders mesh
     *
     * @param vp - (projection * view) matrix from camera
     * @param model - model matrix
     * @param program - shader program for rendering
     */
    void draw(const glm::mat4& vp, const glm::mat4& model, const shared_ptr<grx_shader_program>& program);

    /**
     * @brief Renders many meshes in one OpenGL call
     *
     * @param vp - (projection * view) matrix from camera
     * @param models - vector of model matrices
     * @param program - shader program for rendering
     */
    void
    draw_instanced(const glm::mat4& vp, const vector<glm::mat4>& models, const shared_ptr<grx_shader_program>& program);

    /**
     * @brief Renders mesh
     *
     * @param vp - (projection * view) matrix from camera
     * @param model - model matrix
     * @param tech - shader technique for rendering
     */
    void draw(const glm::mat4& vp, const glm::mat4& model, const class grx_shader_tech& tech);

    /**
     * @brief Renders many meshes in one OpenGL call
     *
     * @param vp - (projection * view) matrix from camera
     * @param models - vector of model matrices
     * @param tech - shader technique for rendering
     */
    void draw_instanced(const glm::mat4& vp, const vector<glm::mat4>& models, const class grx_shader_tech& tech);

private:
    friend class grx_mesh_mgr;

    template <typename T>
    grx_mesh(T&& mesh_vbo, vector<grx_mesh_entry>&& mesh_entries):
        _mesh_vbo(forward<T>(mesh_vbo)), _mesh_entries(move(mesh_entries)) {}

    template <typename T>
    grx_mesh(T&& mesh_vbo, vector<grx_mesh_entry>&& mesh_entries, unique_ptr<grx_bone_data>&& bone_data):
        _mesh_vbo(forward<T>(mesh_vbo)), _mesh_entries(move(mesh_entries)), _bone_data(move(bone_data)) {}

    grx_vbo_tuple_generic      _mesh_vbo;
    vector<grx_mesh_entry>     _mesh_entries;
    vector<grx_texture_set<4>> _texture_sets;
    unique_ptr<grx_bone_data>  _bone_data;
    grx_aabb                   _aabb = grx_aabb();

    class grx_shader_program*        _cached_program = nullptr;
    optional<grx_uniform<glm::mat4>> _cached_uniform_mvp;
    optional<grx_uniform<glm::mat4>> _cached_uniform_model;

    bool _instanced = false;

public:

    /**
     * @brief Gets mesh_vbo
     *
     * @return const reference to mesh_vbo
     */
    [[nodiscard]]
    const grx_vbo_tuple_generic& mesh_vbo() const {
        return _mesh_vbo;
    }

    DECLARE_GET(mesh_entries)
    DECLARE_GET(texture_sets)
    DECLARE_GET(aabb)

    [[nodiscard]]
    const unique_ptr<grx_bone_data>& skeleton() const {
        return _bone_data;
    }
};
} // namespace grx


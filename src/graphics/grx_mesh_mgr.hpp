#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <core/helper_macros.hpp>
#include <core/types.hpp>
#include "grx_types.hpp"
#include "grx_vbo_tuple.hpp"
#include "grx_texture_mgr.hpp"

#define MAX_BONES_COUNT 128

namespace core {
    class config_manager;
}

namespace grx {
    // Simple mesh (no skeleton)
    using mesh_vbo_t = grx_vbo_tuple<
            vbo_vector_indices,  // Index buffer
            vbo_vector_vec3f,    // Position buffer
            vbo_vector_vec2f    // UV buffer
//            vbo_vector_vec3f,    // Normal buffer
//            vbo_vector_vec3f,    // Tangent buffer
//            vbo_vector_vec3f     // Bitangent buffer
            >;

    // Simple mesh, instanced rendering (no skeleton)
    using mesh_instanced_vbo_t = grx_vbo_tuple<
            vbo_vector_indices,  // Index buffer
            vbo_vector_vec3f,    // Position buffer
            vbo_vector_vec2f,    // UV buffer
//            vbo_vector_vec3f,    // Normal buffer
//            vbo_vector_vec3f,    // Tangent buffer
//            vbo_vector_vec3f,    // Bitangent buffer
            vbo_vector_matrix4,  // ModelViewProjection matrix
            vbo_vector_matrix4   // Model matrix
            >;

    using mesh_vbo_skeleton_t = grx_vbo_tuple<
            vbo_vector_indices,  // Index buffer
            vbo_vector_vec3f,    // Position buffer
            vbo_vector_vec2f,    // UV buffer
//            vbo_vector_vec3f,    // Normal buffer
//            vbo_vector_vec3f,    // Tangent buffer
//            vbo_vector_vec3f,    // Bitangent buffer
            vbo_vector_bone
            >;

    using mesh_instenced_vbo_skeleton_t = grx_vbo_tuple<
            vbo_vector_indices,  // Index buffer
            vbo_vector_vec3f,    // Position buffer
            vbo_vector_vec2f,    // UV buffer
//            vbo_vector_vec3f,    // Normal buffer
//            vbo_vector_vec3f,    // Tangent buffer
//            vbo_vector_vec3f,    // Bitangent buffer
            vbo_vector_bone,
            vbo_vector_matrix4,  // ModelViewProjection matrix
            vbo_vector_matrix4   // Model matrix
    >;

    namespace mesh_vbo_types {
        enum mesh_vbo_types {
            INDEX_BUF = 0,
            POSITION_BUF,
            UV_BUF,
//            NORMAL_BUF,
//            TANGENT_BUF,
//            BITANGENT_BUF,
            MVP_MAT,
            MODEL_MAT
        };
    }
    namespace mesh_vbo_skeleton_types {
        enum mesh_vbo_skeleton_types {
            INDEX_BUF = 0,
            POSITION_BUF,
            UV_BUF,
//            NORMAL_BUF,
//            TANGENT_BUF,
//            BITANGENT_BUF,
            BONE_BUF
        };
    }

    struct grx_mesh_entry {
        uint indices_count, material_index = std::numeric_limits<uint>::max(), start_vertex_pos, start_index_pos;
        TO_TUPLE_IMPL(indices_count, material_index, start_vertex_pos, start_index_pos)
    };

    struct anim_position_key {
        double      time;
        core::vec3f value;
    };

    struct anim_scaling_key {
        double      time;
        core::vec3f value;
    };

    struct anim_rotation_key {
        double    time;
        glm::quat value;
    };

    struct anim_channel {
        core::vector<anim_position_key> position_keys;
        core::vector<anim_scaling_key>  scaling_keys;
        core::vector<anim_rotation_key> rotation_keys;
    };

    struct grx_animation {
        double duration, ticks_per_second;
        core::hash_map<core::string, anim_channel> channels;
    };

    struct bone_node {
        core::string name;
        glm::mat4 transform;
        core::vector<core::unique_ptr<bone_node>> children;
    };

    struct grx_bone_data {
        grx_bone_data() {
            std::fill(offsets.begin(), offsets.end(), glm::mat4(1.f));
            std::fill(final_transforms.begin(), final_transforms.end(), glm::mat4(1.f));
        }

        core::hash_map<core::string, grx_animation> animations;
        core::vector<glm::mat4>                     offsets;
        core::vector<glm::mat4>                     final_transforms;
        core::hash_map<core::string, uint>          bone_map;
        core::unique_ptr<bone_node>                 root_node;

        glm::mat4 global_inverse_transform;
        uniform_id_t cached_bone_matrices_uniform = static_cast<uniform_id_t>(-1);
    };

    class grx_mesh {
    public:
        void draw(const core::shared_ptr<class grx_camera>& camera, shader_program_id_t program_id);
        void draw_instanced(const core::shared_ptr<class grx_camera>& camera, shader_program_id_t program_id);

        void set_instance_count(size_t count) {
            if (count < 1)
                count = 1;
            _model_matrices.resize(count, glm::mat4(1.f));
        }

        void set_position(const core::vec3f& pos, size_t index = 0) {
            _model_matrices[index][3].x = pos.x();
            _model_matrices[index][3].y = pos.y();
            _model_matrices[index][3].z = pos.z();
        }

        void translate(const core::vec3f& displacement, size_t index = 0) {
            set_position(position(index) + displacement, index);
        }

        void rotate(const core::vec3f& angles, size_t index = 0) {
            auto rotation = glm::mat4_cast(glm::quat(glm::vec3{angles.x(), angles.y(), angles.z()}));
            _model_matrices[index] *= rotation;
        }

        [[nodiscard]]
        core::vec3f position(size_t index = 0) const {
            return core::vec3f{_model_matrices[index][3].x, _model_matrices[index][3].y, _model_matrices[index][3].z};
        }

    private:
        friend class grx_mesh_mgr;

        template <typename T>
        grx_mesh(T&& mesh_vbo, core::vector<grx_mesh_entry> mesh_entries, size_t instance_count = 1):
                _mesh_vbo(std::forward<T>(mesh_vbo)),
                _mesh_entries(std::move(mesh_entries)),
                _model_matrices(instance_count, glm::mat4(1.f))
        {
            if (instance_count < 1) {
                set_instance_count(1);
                _model_matrices.shrink_to_fit();
            }
        }

        grx_vbo_tuple_generic         _mesh_vbo;
        core::vector<grx_mesh_entry>  _mesh_entries;
        core::vector<grx_texture_set> _texture_sets;
        core::vector<glm::mat4>       _model_matrices = { glm::mat4(1.f) };

        shader_program_id_t _cached_program       = static_cast<shader_program_id_t>(-1);
        uniform_id_t        _cached_uniform_mvp   = static_cast<uniform_id_t>(-1);
        uniform_id_t        _cached_uniform_model = static_cast<uniform_id_t>(-1);

        core::unique_ptr<grx_bone_data> _bone_data;

    public:
        DECLARE_GET(mesh_vbo)
        DECLARE_GET(mesh_entries)
        DECLARE_GET(texture_sets)
    };


    class grx_mesh_mgr {
    public:
        core::shared_ptr<grx_mesh> load(core::string_view path, bool instanced = false);
        core::shared_ptr<grx_mesh> load(const core::config_manager& config_mgr, core::string_view path, bool instanced = false);

    private:
        core::shared_ptr<grx_mesh> load_mesh(core::string_view path, bool instanced);

    private:
        core::hash_map<core::string, core::shared_ptr<grx_mesh>> _meshes;
        grx_texture_mgr _texture_mgr;
    };
} // namespace grx
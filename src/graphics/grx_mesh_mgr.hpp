#pragma once

#include <mutex>
#include <future>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <core/helper_macros.hpp>
#include <core/types.hpp>
#include <core/time.hpp>

#include "grx_types.hpp"
#include "grx_vbo_tuple.hpp"
//#include "grx_texture_mgr.hpp"

#define MAX_BONES_COUNT 128

namespace core {
    class config_manager;
}

class aiScene;

namespace grx {
    using core::tuple;
    using core::vector;
    using core::string;
    using core::string_view;
    using core::hash_map;
    using core::unique_ptr;
    using core::shared_ptr;
    using core::move;
    using core::forward;
    using core::optional;
    using core::vec3f;

    using mesh_data_basic = tuple<
            vector<struct grx_mesh_entry>,
            vbo_vector_indices,  // Index buffer
            vbo_vector_vec3f,    // Position buffer
            vbo_vector_vec2f,    // UV buffer
            vbo_vector_vec3f,    // Normal buffer
            vbo_vector_vec3f,    // Tangent buffer
            vbo_vector_vec3f     // Bitangent buffer
            >;

    using mesh_bone_data = tuple<
            vbo_vector_bone,                    // Bone buffer
            unique_ptr<struct bone_node>, // Skeleton tree
            hash_map<string, uint>, // Name - bone id map
            vector<grx_aabb>,             // Aabbs
            vector<glm::mat4>,            // Offsets matrices
            vector<glm::mat4>             // Final transform matrices
            >;

    struct texture_path_pack {
        optional<string> diffuse;
        optional<string> normal;
        optional<string> specular;
    };

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
        uint indices_count;
        uint vertices_count;
        uint material_index = std::numeric_limits<uint>::max();
        uint start_vertex_pos;
        uint start_index_pos;
        grx_aabb aabb;
    };

    struct anim_position_key {
        double time;
        vec3f  value;
    };

    struct anim_scaling_key {
        double time;
        vec3f  value;
    };

    struct anim_rotation_key {
        double    time;
        glm::quat value;
    };

    struct anim_channel {
        vector<anim_position_key> position_keys;
        vector<anim_scaling_key>  scaling_keys;
        vector<anim_rotation_key> rotation_keys;
    };

    struct grx_animation {
        double duration;
        double ticks_per_second;
        hash_map<string, anim_channel> channels;
    };

    struct bone_node {
        string    name;
        glm::mat4 transform;
        vector<unique_ptr<bone_node>> children;
    };

    class grx_bone_data {
    protected:
        friend class grx_mesh_mgr;
        friend class grx_mesh;
        friend class grx_mesh_instance;

        static void skeleton_aabb_traverse(grx_bone_data& bone_data);
        static grx_aabb calculate_skeleton_aabb(grx_bone_data& bone_data);

        grx_bone_data() {
            std::fill(offsets.begin(), offsets.end(), glm::mat4(1.f));
            std::fill(final_transforms.begin(), final_transforms.end(), glm::mat4(1.f));
        }

    private:
        hash_map<string, grx_animation> animations;
        vector<glm::mat4>               offsets;
        vector<glm::mat4>               final_transforms;
        vector<grx_aabb>                aabbs;
        hash_map<string, uint>          bone_map;
        unique_ptr<bone_node>           root_node;

        glm::mat4 global_inverse_transform;
        uniform_id_t cached_bone_matrices_uniform = static_cast<uniform_id_t>(-1);
    };

    class grx_mesh {
    public:
        void draw          (const glm::mat4& vp, const glm::mat4& model,          shader_program_id_t program_id);
        void draw_instanced(const glm::mat4& vp, const vector<glm::mat4>& models, shader_program_id_t program_id);

        void draw          (const glm::mat4& vp, const glm::mat4& model,          const class grx_shader_tech& tech);
        void draw_instanced(const glm::mat4& vp, const vector<glm::mat4>& models, const class grx_shader_tech& tech);

    private:
        friend class grx_mesh_mgr;

        // TODO: remove this later
        //template <typename T>
        //grx_mesh(T&& mesh_vbo,
        //         vector<grx_mesh_entry>&& mesh_entries
        //):  _mesh_vbo    (forward<T>(mesh_vbo)),
        //    _mesh_entries(move(mesh_entries))
        //{}

        template <typename T>
        grx_mesh(T&& mesh_vbo,
                 vector<grx_mesh_entry>&& mesh_entries//,
                 //vector<grx_texture_set>&& texture_sets
        ):  _mesh_vbo    (forward<T>(mesh_vbo)),
            _mesh_entries(move(mesh_entries))//,
            //_texture_sets(move(texture_sets))
        {}

        template <typename T>
        grx_mesh(T&& mesh_vbo,
                 vector<grx_mesh_entry>&& mesh_entries,
                 //vector<grx_texture_set>&& texture_sets,
                 unique_ptr<grx_bone_data>&& bone_data
        ):  _mesh_vbo    (forward<T>(mesh_vbo)),
            _mesh_entries(move(mesh_entries)),
            //_texture_sets(move(texture_sets)),
            _bone_data   (move(bone_data))
        {}

        grx_vbo_tuple_generic     _mesh_vbo;
        vector<grx_mesh_entry>    _mesh_entries;
        //vector<grx_texture_set>   _texture_sets;
        unique_ptr<grx_bone_data> _bone_data;
        grx_aabb                  _aabb;

        shader_program_id_t _cached_program       = static_cast<shader_program_id_t>(-1);
        uniform_id_t        _cached_uniform_mvp   = static_cast<uniform_id_t>(-1);
        uniform_id_t        _cached_uniform_model = static_cast<uniform_id_t>(-1);

        bool _instanced = false;

    public:
        DECLARE_GET(mesh_vbo)
        DECLARE_GET(mesh_entries)
        //DECLARE_GET(texture_sets)
        DECLARE_GET(aabb)

        const unique_ptr<grx_bone_data>& skeleton() const {
            return _bone_data;
        }
    };


    class grx_mesh_mgr {
    private:
        static void load_anim(const aiScene* scene, grx_bone_data& bone_data);

    public:
        shared_ptr<grx_mesh> load(string_view path, bool instanced = false);
        shared_ptr<grx_mesh> load(const core::config_manager& config_mgr, string_view path, bool instanced = false);

        static void anim_traverse(
                grx_bone_data& bone_data,
                const grx_animation& anim,
                double time,
                const bone_node* node,
                const glm::mat4& parent_transform = glm::mat4(1.f));

    private:
        static shared_ptr<grx_mesh> load_mesh(string_view path, bool instanced/*, grx_texture_mgr* texture_mgr*/);

    private:
        std::mutex _meshes_mutex;
        hash_map<string, shared_ptr<grx_mesh>> _meshes;
        //grx_texture_mgr _texture_mgr;
    };

} // namespace grx


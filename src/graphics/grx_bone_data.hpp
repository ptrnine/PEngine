#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "grx_shader.hpp"
#include "grx_types.hpp"

namespace grx
{
using core::hash_map;
using core::optional;
using core::string;
using core::unique_ptr;
using core::vec3f;
using core::vector;

/**
 * @brief Position animation key data
 */
struct anim_position_key {
    double time;
    vec3f  value;
};

/**
 * @brief Scaling animation key data
 */
struct anim_scaling_key {
    double time;
    vec3f  value;
};

/**
 * @brief Rotation animation key data
 */
struct anim_rotation_key {
    double    time;
    glm::quat value;
};

/**
 * @brief Stores data specific for the animation channel
 */
struct anim_channel {
    vector<anim_position_key> position_keys;
    vector<anim_scaling_key>  scaling_keys;
    vector<anim_rotation_key> rotation_keys;
};

/**
 * @brief Stores animation specific data
 */
struct grx_animation { // NOLINT
    double                         duration;
    double                         ticks_per_second;
    hash_map<string, anim_channel> channels;
};

/**
 * @brief Stores bone specific data
 */
struct bone_node {
    string                        name;
    glm::mat4                     transform;
    vector<unique_ptr<bone_node>> children;
};

/**
 * @brief Stores skeleton specific data
 */
struct grx_bone_data {
    static void     skeleton_aabb_traverse(grx_bone_data& bone_data);
    static grx_aabb calculate_skeleton_aabb(grx_bone_data& bone_data);

    grx_bone_data() {
        std::fill(offsets.begin(), offsets.end(), glm::mat4(1.f));
        std::fill(final_transforms.begin(), final_transforms.end(), glm::mat4(1.f));
    }

    hash_map<string, grx_animation> animations;
    vector<glm::mat4>               offsets;
    vector<glm::mat4>               final_transforms;
    vector<grx_aabb>                aabbs;
    hash_map<string, uint>          bone_map;
    unique_ptr<bone_node>           root_node;

    glm::mat4                                    global_inverse_transform = glm::mat4(1.f);
    optional<grx_uniform<core::span<glm::mat4>>> cached_bone_matrices_uniform;
};
} // namespace grx


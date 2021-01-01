#pragma once

#include <core/serialization.hpp>
#include <core/assert.hpp>
#include "grx_types.hpp"

constexpr core::u32 MAX_BONES_PER_VERTEX = 4;

namespace grx
{
using vbo_vector_vec2f = core::vector<core::vec2f>;
template <typename T>
concept VboVectorVec2f = std::is_same_v<T, vbo_vector_vec2f>;

using vbo_vector_vec3f = core::vector<core::vec3f>;
template <typename T>
concept VboVectorVec3f = std::is_same_v<T, vbo_vector_vec3f>;

using vbo_vector_indices = core::vector<uint>;
template <typename T>
concept VboVectorIndices = std::is_same_v<T, vbo_vector_indices>;

using vbo_vector_matrix4 = core::vector<glm::mat4>;
template <typename T>
concept VboVectorMatrix4 = std::is_same_v<T, vbo_vector_matrix4>;


struct grx_bone_vertex_data {
    PE_SERIALIZE(ids, weights)

    grx_bone_vertex_data() { // NOLINT
        std::fill(ids.begin(), ids.end(), 0);
        std::fill(weights.begin(), weights.end(), 0.f);
    }

    [[nodiscard]]
    static constexpr size_t size() {
        return MAX_BONES_PER_VERTEX;
    }

    void append(uint bone_id, float weight) {
        float zero = 0.0f;
        for (size_t i = 0; i < size(); ++i) {
            if (!memcmp(&weights[i], &zero, sizeof(float))) {
                ids[i]     = bone_id;
                weights[i] = weight;
                return;
            }
        }

        PeAbortF("Not enough bone size. Weights: {}", weights);
    }

    core::array<core::u32, MAX_BONES_PER_VERTEX> ids;
    core::array<float, MAX_BONES_PER_VERTEX>     weights;
};

using vbo_vector_bone = core::vector<grx_bone_vertex_data>;

template <typename T>
concept VboVectorBone = std::is_same_v<T, vbo_vector_bone>;

template <typename T>
concept VboData = VboVectorVec2f<T> || VboVectorVec3f<T> || VboVectorIndices<T> ||
                  VboVectorMatrix4<T> || VboVectorBone<T>;

} // namespace grx

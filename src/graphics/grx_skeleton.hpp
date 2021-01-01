#pragma once

#include "grx_types.hpp"
#include <core/assert.hpp>
#include "grx_vbo_types.hpp"

class aiScene;

namespace grx {

constexpr core::u32 MAX_BONES_COUNT = 128;

struct grx_bone_node {
    core::string                                  name;
    glm::mat4                                     transform;
    core::vector<core::unique_ptr<grx_bone_node>> children;
};

struct grx_skeleton_data {
    core::hash_map<core::string, core::u32> mapping;
    core::vector<grx_aabb>                  aabbs;
    core::vector<glm::mat4>                 offsets;
    core::vector<glm::mat4>                 final_transforms;
};

struct grx_bone_node_optimized {
    glm::mat4                           transform;
    glm::mat4                           offset;
    core::span<grx_bone_node_optimized> children;
    grx_aabb                            aabb;
    core::u32                           idx;
};

core::unique_ptr<grx_bone_node> assimp_extract_nodes(const aiScene* scene);
grx_skeleton_data assimp_extract_skeleton_data(const aiScene* scene);

class grx_skeleton_optimized;

class grx_skeleton {
public:
    grx_skeleton(core::unique_ptr<grx_bone_node>&& root, grx_skeleton_data skeleton_data):
        _root(core::move(root)), _skeleton_data(core::move(skeleton_data)) {}

    static grx_skeleton from_assimp(const aiScene* assimpScene);

    [[nodiscard]]
    grx_skeleton_optimized get_optimized() const;

    [[nodiscard]]
    const core::unique_ptr<grx_bone_node>& root() const {
        return _root;
    }

    [[nodiscard]]
    const grx_skeleton_data& skeleton_data() const {
        return _skeleton_data;
    }

    void traverse(core::function<void(const grx_bone_node&)> callback) const;
    void traverse(core::function<void(const grx_bone_node&, core::u32)> callback) const;
    void traverse(core::function<void(grx_bone_node&)> callback);
    void traverse(core::function<void(grx_bone_node&, core::u32)> callback);

private:
    core::unique_ptr<grx_bone_node> _root;
    grx_skeleton_data               _skeleton_data;
};


class grx_skeleton_optimized {
public:
    void serialize(core::vector<core::byte>& out) const;
    void deserialize(core::span<const core::byte>& in);

    grx_skeleton_optimized() = default;
    ~grx_skeleton_optimized() = default;

    /* TODO: impement it */
    grx_skeleton_optimized(const grx_skeleton_optimized&) = delete;
    grx_skeleton_optimized& operator=(const grx_skeleton_optimized&) = delete;

    grx_skeleton_optimized(grx_skeleton_optimized&&) = default;
    grx_skeleton_optimized& operator=(grx_skeleton_optimized&&) = default;

    grx_skeleton_optimized(const core::unique_ptr<grx_bone_node>& root,
                           const grx_skeleton_data&               mapping);

    grx_skeleton_optimized(const grx_skeleton& skeleton):
        grx_skeleton_optimized(skeleton.root(), skeleton.skeleton_data()) {}

    void traverse(core::function<void(const grx_bone_node_optimized&)> callback) const;
    void traverse(core::function<void(const grx_bone_node_optimized&, const glm::mat4&)> callback) const;
    void traverse(core::function<void(const grx_bone_node_optimized&, core::u32)> callback) const;
    void traverse(core::function<void(grx_bone_node_optimized&)> callback);
    void traverse(core::function<void(grx_bone_node_optimized&, core::u32)> callback);

    [[nodiscard]]
    core::vector<glm::mat4> animation_transforms(const class grx_animation_optimized& animation, double time) const;

    [[nodiscard]]
    auto& storage() const {
        return _storage;
    }

    [[nodiscard]]
    auto& final_transforms() const {
        return _final_transforms;
    }

    [[nodiscard]]
    core::u32 depth() const {
        return _depth;
    }

private:
    core::vector<grx_bone_node_optimized> _storage;
    core::vector<glm::mat4>               _final_transforms;
    core::u32 _depth;
};


inline size_t calc_nodes_count(const core::unique_ptr<grx_bone_node>& root) {
    size_t count = 1;
    for (auto& child : root->children)
        count += calc_nodes_count(child);
    return count;
}

}

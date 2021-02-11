#pragma once

#include "graphics/grx_debug.hpp"
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
    void traverse(core::function<void(const grx_bone_node_optimized&, core::u32)> callback) const;
    void traverse(core::function<void(grx_bone_node_optimized&)> callback);
    void traverse(core::function<void(grx_bone_node_optimized&, core::u32)> callback);

    [[nodiscard]] core::vector<glm::mat4>
    animation_transforms(const class grx_animation_optimized& animation, double time) const;

    [[nodiscard]] core::vector<glm::mat4>
    animation_factor_transforms(const class grx_animation_optimized& animation,
                                double                               factor) const;

    [[nodiscard]] core::vector<glm::mat4>
    animation_interpolate_transform(const class grx_animation_optimized& animation_start,
                                    const class grx_animation_optimized& animation_end,
                                    double                               animation_start_time,
                                    double                               animation_end_time,
                                    double                               factor) const;

    [[nodiscard]] core::vector<glm::mat4>
    animation_interpolate_factor_transform(const class grx_animation_optimized& animation_start,
                                           const class grx_animation_optimized& animation_end,
                                           double animation_start_factor,
                                           double animation_end_factor,
                                           double factor) const;

    [[nodiscard]]
    grx_aabb calc_aabb(const core::vector<glm::mat4>& final_transforms) {
        Expects(_final_transforms.size() == final_transforms.size());

        auto result = grx_aabb::maximized();

        traverse([&](const grx_bone_node_optimized& node) {
            auto aabb = node.aabb;

            aabb.transform(final_transforms[node.idx]);
            result.merge(aabb);
        });

        return result;
    }

    [[nodiscard]]
    grx_aabb calc_aabb(const glm::mat4&            model_mat,
                       core::span<const glm::mat4> final_transforms) const {
        Expects(_final_transforms.size() == static_cast<size_t>(final_transforms.size()));

        auto result_aabb = grx_aabb::maximized();

        traverse([&](const grx_bone_node_optimized& node) {
            auto aabb = node.aabb;

            aabb.transform(final_transforms[node.idx]);
            aabb.transform(model_mat);
            result_aabb.merge(aabb);
        });

        return result_aabb;
    }

    [[nodiscard]]
    grx_aabb calc_aabb(const glm::mat4& model_mat) const {
        return calc_aabb(model_mat, _final_transforms);
    }

    void debug_draw_aabbs(const glm::mat4&            model_mat,
                          core::span<const glm::mat4> final_transforms) const {
        Expects(_final_transforms.size() == static_cast<size_t>(final_transforms.size()));

        if (!grx_aabb_debug().is_enabled())
            return;

        auto result_aabb = grx_aabb::maximized();

        traverse([&](const grx_bone_node_optimized& node) {
            auto aabb = node.aabb;

            aabb.transform(final_transforms[node.idx]);
            result_aabb.merge(aabb);

            grx_aabb_debug().push(aabb, model_mat, grx::color_rgb{127, 0, 255});
        });

        grx_aabb_debug().push(result_aabb, model_mat, grx::color_rgb{255, 0, 100});
    }

    void debug_draw_aabbs(const glm::mat4& model_mat) const {
        debug_draw_aabbs(model_mat, _final_transforms);
    }

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

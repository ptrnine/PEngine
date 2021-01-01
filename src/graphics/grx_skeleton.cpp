#include "grx_skeleton.hpp"

#include <core/serialization.hpp>
#include <core/string_hash.hpp>
#include <assimp/scene.h>
#include "grx_animation.hpp"

using namespace core;

namespace {
using bone_opt_iter = vector<grx::grx_bone_node_optimized>::iterator;

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

u32 build_bone_tree_optimized(vector<grx::grx_bone_node_optimized>&  result,
                               const unique_ptr<grx::grx_bone_node>& root,
                               const grx::grx_skeleton_data&         skeleton_data,
                               size_t&                               idx,
                               size_t                                app = 0,
                               u32                                   depth = 1) {
    auto& it = result[app];

    it.children  = span(result.data() + idx, static_cast<ptrdiff_t>(root->children.size()));
    it.transform = root->transform;
    it.idx       = skeleton_data.mapping.at(root->name);
    it.offset    = skeleton_data.offsets[it.idx];
    it.aabb      = skeleton_data.aabbs[it.idx];

    auto start = idx;
    idx += root->children.size();

    for (const auto& node : root->children) {
        auto new_depth = build_bone_tree_optimized(result, node, skeleton_data, idx, start++, depth + 1);
        if (new_depth > depth)
            depth = new_depth;
    }

    return depth;
}

void load_node_iter(grx::grx_bone_node* dst, aiNode* src) {
    dst->name      = src->mName.data;
    dst->transform = to_glm(src->mTransformation);

    dst->children.resize(src->mNumChildren);
    auto children = span(src->mChildren, src->mNumChildren);

    for (auto& [dst1, src1] : zip_view(dst->children, children)) {
        dst1 = make_unique<grx::grx_bone_node>();
        load_node_iter(dst1.get(), src1);
    }
}

void remove_orphan_non_bone_nodes(unique_ptr<grx::grx_bone_node>& node, const grx::grx_skeleton_data& skeleton_data) {
    vector<unique_ptr<grx::grx_bone_node>> bone_nodes;

    for (auto& child : node->children) {
        remove_orphan_non_bone_nodes(child, skeleton_data);
        if (!child->children.empty() || skeleton_data.mapping.count(child->name))
            bone_nodes.emplace_back(move(child));
    }

    node->children = move(bone_nodes);
}

unique_ptr<grx::grx_bone_node>& prune_non_bone_root(unique_ptr<grx::grx_bone_node>& node,
                                                   const grx::grx_skeleton_data&   skeleton_data) {
    if (skeleton_data.mapping.count(node->name) == 0 && node->children.size() == 1)
        return prune_non_bone_root(node->children.front(), skeleton_data);
    else
        return node;
}

void insert_identity_for_non_bones(const unique_ptr<grx::grx_bone_node>& node,
                                   grx::grx_skeleton_data&               skeleton_data) {
    for (auto& child : node->children)
        insert_identity_for_non_bones(child, skeleton_data);

    auto [position, was_inserted] = skeleton_data.mapping.emplace(node->name, skeleton_data.offsets.size());

    if (was_inserted) {
        Expects(!node->children.empty());

        auto& [mapping, aabbs, offsets, final_transforms] = skeleton_data;

        offsets.emplace_back(glm::mat4{1.f});
        final_transforms.emplace_back(glm::mat4{1.f});

        auto merged_aabb = grx::grx_aabb::maximized();
        for (auto& child : node->children)
            merged_aabb.merge(aabbs[mapping.at(child->name)]);
        aabbs.emplace_back(merged_aabb);
    }
}

void node_traverse(const unique_ptr<grx::grx_bone_node>& node, function<void(const grx::grx_bone_node&)>& callback) {
    callback(*node);
    for (auto& child : node->children)
        node_traverse(child, callback);
}

void node_traverse(const unique_ptr<grx::grx_bone_node>&           node,
                   function<void(const grx::grx_bone_node&, u32)>& callback,
                   core::u32                                       depth = 0) {
    callback(*node, depth);
    for (auto& child : node->children)
        node_traverse(child, callback, depth + 1);
}

void node_traverse(const unique_ptr<grx::grx_bone_node>& node, function<void(grx::grx_bone_node&)>& callback) {
    callback(*node);
    for (auto& child : node->children)
        node_traverse(child, callback);
}

void node_traverse(const unique_ptr<grx::grx_bone_node>&     node,
                   function<void(grx::grx_bone_node&, u32)>& callback,
                   core::u32                                 depth = 0) {
    callback(*node, depth);
    for (auto& child : node->children) node_traverse(child, callback, depth + 1);
}

void node_traverse(const grx::grx_bone_node_optimized&                  node,
                   function<void(const grx::grx_bone_node_optimized&)>& callback) {
    callback(node);
    for (auto& child : node.children)
        node_traverse(child, callback);
}

void node_traverse(const grx::grx_bone_node_optimized&                       node,
                   function<void(const grx::grx_bone_node_optimized&, u32)>& callback,
                   u32                                                       depth = 0) {
    callback(node, depth);
    for (auto& child : node.children)
        node_traverse(child, callback, depth + 1);
}

void node_traverse(grx::grx_bone_node_optimized&                  node,
                   function<void(grx::grx_bone_node_optimized&)>& callback) {
    callback(node);
    for (auto& child : node.children)
        node_traverse(child, callback);
}

void node_traverse(grx::grx_bone_node_optimized&                       node,
                   function<void(grx::grx_bone_node_optimized&, u32)>& callback,
                   u32                                                 depth = 0) {
    callback(node, depth);
    for (auto& child : node.children)
        node_traverse(child, callback, depth + 1);
}

void anim_traverse(const grx::grx_bone_node_optimized& node,
                   const grx::grx_animation_optimized& animation,
                   double                              time,
                   const glm::mat4&                    global_inverse_transform,
                   core::span<glm::mat4>               final_transforms,
                   const glm::mat4&                    parent_transform = glm::mat4(1.0)) {
    auto& channel = animation.channels().at(node.idx);
    auto [position, scaling, rotation] =
        grx::grx_animation_key_lookup(channel).interstep(time).interpolate();
    auto t   = glm::translate(glm::mat4(1.0), to_glm(position));
    auto s   = glm::scale(glm::mat4(1.0), to_glm(scaling));
    auto r   = glm::mat4_cast(rotation);
    auto tsr = t * s * r;
    auto global_transform = parent_transform * tsr;
    final_transforms[node.idx] = global_inverse_transform * global_transform * node.offset;

    for (auto& child : node.children)
        anim_traverse(child, animation, time, global_inverse_transform, final_transforms, global_transform);
}

} // namespace


namespace grx {

unique_ptr<grx_bone_node> assimp_extract_nodes(const aiScene* scene) {
    auto root = make_unique<grx_bone_node>();
    ::load_node_iter(root.get(), scene->mRootNode);
    return root;
}

grx_skeleton_data assimp_extract_skeleton_data(const aiScene* scene) {
    if (!scene->HasMeshes())
        throw std::runtime_error("Mesh does not have meshes");

    grx_skeleton_data result;
    auto& [mapping, aabbs, offsets, final_transforms] = result;

    auto src_meshes = span{scene->mMeshes, scene->mNumMeshes};
    for (auto& [mesh, i] : value_index_view(src_meshes)) {
        for (auto bone : span{mesh->mBones, mesh->mNumBones}) {
            auto [position, was_inserted] = mapping.emplace(bone->mName.data, offsets.size());

            if (was_inserted) {
                offsets.emplace_back(to_glm(bone->mOffsetMatrix));
                final_transforms.emplace_back(glm::mat4{1.f});
                aabbs.emplace_back(grx_aabb::maximized());
            }

            auto [_, index] = *position;
            auto& aabb = aabbs[index];

            /* Update aabb for bone */
            for (auto& weight : span(bone->mWeights, bone->mNumWeights)) {
                auto vertex = to_vec3f(mesh->mVertices[weight.mVertexId]);
                aabb.merge({vertex, vertex});
            }
        }
    }

    return result;
}

grx_skeleton grx_skeleton::from_assimp(const aiScene* scene) {
    auto skeleton = grx_skeleton(assimp_extract_nodes(scene), assimp_extract_skeleton_data(scene));

    remove_orphan_non_bone_nodes(skeleton._root, skeleton._skeleton_data);
    auto new_root  = move(prune_non_bone_root(skeleton._root, skeleton._skeleton_data));
    skeleton._root = move(new_root);

    insert_identity_for_non_bones(skeleton._root, skeleton._skeleton_data);

    return skeleton;
}

void grx_skeleton::traverse(function<void(const grx_bone_node&)> callback) const {
    if (_root)
        node_traverse(_root, callback);
}

void grx_skeleton::traverse(function<void(const grx_bone_node&, u32)> callback) const {
    if (_root)
        node_traverse(_root, callback);
}

void grx_skeleton::traverse(function<void(grx_bone_node&)> callback) {
    if (_root)
        node_traverse(_root, callback);
}

void grx_skeleton::traverse(function<void(grx_bone_node&, u32)> callback) {
    if (_root)
        node_traverse(_root, callback);
}

grx_skeleton_optimized grx_skeleton::get_optimized() const {
    return grx_skeleton_optimized(_root, _skeleton_data);
}

void grx_skeleton_optimized::serialize(byte_vector& out) const {
    core::serialize(static_cast<u64>(_storage.size()), out);
    for (auto& node : _storage) {
        core::serialize(node.aabb, out);
        core::serialize(static_cast<span<const grx_bone_node_optimized>>(node.children), _storage.data(), out);
        core::serialize_all(out, node.idx, node.offset, node.transform);
    }

    core::serialize_all(out, _final_transforms, _depth);
}

void grx_skeleton_optimized::deserialize(span<const byte>& in) {
    u64 size;
    core::deserialize(size, in);
    _storage.resize(size);

    for (size_t i = 0; i < _storage.size(); ++i) {
        auto& node = _storage[i];
        core::deserialize(node.aabb, in);
        core::deserialize(node.children, _storage.data(), in);
        core::deserialize_all(in, node.idx, node.offset, node.transform);
    }

    core::deserialize_all(in, _final_transforms, _depth);
}

grx_skeleton_optimized::grx_skeleton_optimized(const unique_ptr<grx_bone_node>& root,
                                               const grx_skeleton_data&         skeleton_data):
    _final_transforms(skeleton_data.final_transforms) {

    _storage = vector<grx_bone_node_optimized>{calc_nodes_count(root)};
    size_t idx = 1;
    _depth = ::build_bone_tree_optimized(_storage, root, skeleton_data, idx);

    /* Remap indices */
    /*
    auto old_to_new = vector<u32>(_final_transforms.size());
    auto new_to_old = vector<u32>(_final_transforms.size());
    u32 counter = 0;
    auto f = function{[&](const grx_bone_node_optimized& n) { old_to_new[n.idx] = counter; new_to_old[counter++] = n.idx; }};
    node_traverse(_storage.front(), f);

    for (auto& [dst, old_idx] : zip_view(_final_transforms, new_to_old))
        dst = skeleton_data.final_transforms[old_idx];

    auto remap = function{[&](grx_bone_node_optimized& n) { n.idx = old_to_new[n.idx]; }};
    node_traverse(_storage.front(), remap);
    */
}

void grx_skeleton_optimized::traverse(core::function<void(const grx_bone_node_optimized&)> callback) const {
    if (!_storage.empty())
        node_traverse(_storage.front(), callback);
}

void grx_skeleton_optimized::traverse(core::function<void(const grx_bone_node_optimized&, u32)> callback) const {
    if (!_storage.empty())
        node_traverse(_storage.front(), callback);
}

void grx_skeleton_optimized::traverse(core::function<void(grx_bone_node_optimized&)> callback) {
    if (!_storage.empty())
        node_traverse(_storage.front(), callback);
}

void grx_skeleton_optimized::traverse(core::function<void(grx_bone_node_optimized&, u32)> callback) {
    if (!_storage.empty())
        node_traverse(_storage.front(), callback);
}

vector<glm::mat4>
grx_skeleton_optimized::animation_transforms(const grx_animation_optimized& animation,
                                             double                         time) const {
    auto result = vector<glm::mat4>(_final_transforms.size());
    anim_traverse(
        _storage.front(), animation, time, glm::inverse(_storage.front().transform), result);
    return result;
}
} // namespace grx

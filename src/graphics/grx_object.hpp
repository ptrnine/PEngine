#pragma once

#include "graphics/grx_mesh.hpp"
#include "graphics/grx_texture_path_set.hpp"
#include "grx_animation.hpp"
#include "grx_vbo_tuple.hpp"
#include "grx_cpu_mesh_group.hpp"
#include "grx_uniform_cache.hpp"
#include "grx_texture_set.hpp"
#include "grx_shader_tech.hpp"

namespace grx {

class grx_object_skeleton {
public:
    PE_SERIALIZE(_skeleton, _animations)

    void set_skeleton(grx_skeleton_optimized skeleton) {
        _skeleton = core::move(skeleton);
    }

    void set_animation(core::string animation_name, grx_animation_optimized animation) {
        _animations.emplace(core::move(animation_name), core::move(animation));
    }

    void set_animations(core::hash_map<core::string, grx_animation_optimized> animations) {
        _animations = core::move(animations);
    }

    grx_skeleton_optimized                                _skeleton;
    core::hash_map<core::string, grx_animation_optimized> _animations;
};

using grx_object_instance_id = core::u64;

class grx_object_instance_storage {
public:
    PE_SERIALIZE(_current_id, _mvp_matrices, _m_matrices, _id_to_instance)

    struct instance_value {
        PE_SERIALIZE(mvp, m)

        glm::mat4 mvp;
        glm::mat4 m;
    };

    grx_object_instance_id create_instance() {
        _id_to_instance.emplace(_current_id, instance_value{glm::mat4(1.f), glm::mat4(1.f)});
        return _current_id++;
    }

    void delete_instance(grx_object_instance_id id) {
        _id_to_instance.erase(id);
    }

    instance_value& get_instance(grx_object_instance_id id) {
        return _id_to_instance.at(id);
    }

    const instance_value& get_instance(grx_object_instance_id id) const {
        return _id_to_instance.at(id);
    }

    [[nodiscard]]
    bool has_instance(grx_object_instance_id id) const {
        return _id_to_instance.count(id);
    }

    void setup_instance_buffers() {
        _mvp_matrices.resize(_id_to_instance.size());
        _m_matrices.resize(_id_to_instance.size());

        for (size_t i = 0; auto& [_, instance_val] : _id_to_instance) {
            _mvp_matrices[i] = instance_val.mvp;
            _m_matrices[i] = instance_val.m;
            ++i;
        }
    }

    grx_object_instance_id _current_id = 0;
    core::vector<glm::mat4> _mvp_matrices;
    core::vector<glm::mat4> _m_matrices;
    core::hash_map<grx_object_instance_id, instance_value> _id_to_instance;
};

namespace details {
    template <bool HasSkeleton>
    struct skeleton_storage {};

    template <>
    struct skeleton_storage<true> : public grx_object_skeleton {};

    template <bool IsInstanced>
    struct instance_storage {};

    template <>
    struct instance_storage<true> : public grx_object_instance_storage {};

    template <typename T, typename... Ts>
    constexpr bool has_type() {
        return false || ((std::is_same_v<T, Ts>) || ...);
    }
}

struct grx_object_instanced_tag{};

template <bool IsInstanced, typename MeshT, typename... Ts>
class grx_object
    : public details::skeleton_storage<details::has_type<core::vector<grx_bone_vertex_data>, Ts...>()>,
      public details::instance_storage<IsInstanced> {
public:
    static constexpr bool is_instanced() {
        return IsInstanced;
    }

    static constexpr bool has_skeleton() {
        return details::has_type<core::vector<grx_bone_vertex_data>, Ts...>();
    }

    template <bool Instanced = IsInstanced, bool Skeleton = has_skeleton()>
    void serialize(core::vector<core::byte>& s) const {
        core::serialize_all(s, to_mesh_group(), _texture_sets);
        if constexpr (Skeleton)
            grx_object_skeleton::serialize(s);
        if constexpr (Instanced)
            grx_object_instance_storage::serialize(s);
    }

    template <bool Instanced = IsInstanced, bool Skeleton = has_skeleton()>
    void deserialize(core::span<const core::byte>& d) {
        MeshT cpu_mesh;
        core::deserialize_all(d, cpu_mesh, _texture_sets);

        set_from_mesh_group(cpu_mesh);

        if constexpr (Skeleton)
            grx_object_skeleton::deserialize(d);
        if constexpr (Instanced)
            grx_object_instance_storage::deserialize(d);
    }

    grx_object() = default;

    template <MeshBufT... BufTs>
    grx_object(const grx_cpu_mesh_group<BufTs...>& mesh_group):
        grx_object(mesh_group, std::make_index_sequence<sizeof...(BufTs)>()) {}

    template <MeshBufT... BufTs>
    grx_object(const grx_cpu_mesh_group<BufTs...>& mesh_group, grx_object_instanced_tag&&):
        grx_object(mesh_group, std::make_index_sequence<sizeof...(BufTs)>()) {}

    template <bool Enable = !IsInstanced, bool Skeleton = has_skeleton()>
    std::enable_if_t<Enable> draw(const glm::mat4&                            vp,
                                  const glm::mat4&                            model,
                                  const core::shared_ptr<grx_shader_program>& program,
                                  bool enable_textures = true) {
        program->activate();

        _unicache.setup(program);

        if (auto& mvp = _unicache.get<unitag_mvp>())
            *mvp = vp * model;

        if (auto& m = _unicache.get<unitag_model>())
            *m = model;

        if constexpr (Skeleton) {
            if (auto& bone_matrices = _unicache.get<unitag_bone_matrices>())
                *bone_matrices = this->_skeleton.final_transforms();
        }

        _vbo.bind_vao();
        for (auto& element : _elements) {
            if (enable_textures && element.material_index != core::numlim<uint>::max()) {
                auto diffuse_ptr = _texture_sets[element.material_index].get_ptr(grx_texture_set_tag::diffuse);
                if (diffuse_ptr) {
                    auto diffuse = diffuse_ptr->try_access();
                    if (auto& texture0 = _unicache.get<unitag_texture0>(); texture0 && diffuse) {
                        diffuse->bind_unit(0);
                        *texture0 = 0;
                    }
                }

                auto normal_ptr = _texture_sets[element.material_index].get_ptr(grx_texture_set_tag::normal);
                if (normal_ptr) {
                    auto normal = normal_ptr->try_access();
                    if (auto& texture1 = _unicache.get<unitag_texture1>(); texture1 && normal) {
                        normal->bind_unit(1);
                        *texture1 = 1;
                    }
                }
            }

            _vbo.draw(element.indices_count, element.start_vertex_pos, element.start_index_pos);
        }
    }

    template <bool Enable = IsInstanced, bool Skeleton = has_skeleton()>
    std::enable_if_t<Enable> draw(const glm::mat4&                            vp,
                                  const core::shared_ptr<grx_shader_program>& program,
                                  bool enable_textures = true) {
        if (this->_m_matrices.empty())
            return;

        program->activate();

        _unicache.setup(program);

        if constexpr (Skeleton) {
            if (auto& bone_matrices = _unicache.get<unitag_bone_matrices>())
                *bone_matrices = this->_skeleton.final_transforms();
        }

        this->setup_instance_buffers();

        _vbo.bind_vao();
        for (auto& element : _elements) {
            if (enable_textures && element.material_index != core::numlim<uint>::max()) {
                auto diffuse_ptr = _texture_sets[element.material_index].get_ptr(grx_texture_set_tag::diffuse);
                if (diffuse_ptr) {
                    auto diffuse = diffuse_ptr->try_access();
                    if (auto& texture0 = _unicache.get<unitag_texture0>(); texture0 && diffuse) {
                        diffuse->bind_unit(0);
                        *texture0 = 0;
                    }
                }

                auto normal_ptr = _texture_sets[element.material_index].get_ptr(grx_texture_set_tag::normal);
                if (normal_ptr) {
                    auto normal = normal_ptr->try_access();
                    if (auto& texture1 = _unicache.get<unitag_texture1>(); texture1 && normal) {
                        normal->bind_unit(1);
                        *texture1 = 1;
                    }
                }
            }

            _vbo.draw_instanced(this->_m_matrices.size(),
                                element.indices_count,
                                element.start_vertex_pos,
                                element.start_index_pos);
        }
    }

    template <bool Enable = !IsInstanced, bool Skeleton = has_skeleton()>
    std::enable_if_t<Enable> draw(const glm::mat4&       vp,
                                  const glm::mat4&       model,
                                  const grx_shader_tech& tech,
                                  bool                   enable_textures = true) {
        if constexpr (Skeleton)
            draw(vp, model, tech.skeleton(), enable_textures);
        else
            draw(vp, model, tech.base(), enable_textures);
    }

    template <bool Enable = IsInstanced, bool Skeleton = has_skeleton()>
    std::enable_if_t<Enable>
    draw(const glm::mat4& vp, const grx_shader_tech& tech, bool enable_textures = true) {
        if constexpr (Skeleton)
            draw(vp, tech.skeleton(), enable_textures); // TODO: replace for skeleton_instanced()
        else
            draw(vp, tech.instanced(), enable_textures);
    }

    void set_from_mesh_group(const MeshT& mesh_group) {
        _set_from_mesh_group(mesh_group);
    }

    MeshT to_mesh_group() const {
        return to_mesh_group(std::make_index_sequence<MeshT::buffers_count>());
    }

    void set_textures(core::vector<grx_texture_set<4>> texture_sets) {
        _texture_sets = core::move(texture_sets);
    }

private:
    template <MeshBufT... BufTs, size_t... Idxs>
    grx_object(const grx_cpu_mesh_group<BufTs...>& mesh_group, std::index_sequence<Idxs...>&&):
        _elements(mesh_group.elements()) {
        ((_vbo.template set_data<Idxs>(mesh_group.template get<BufTs::tag>())), ...);
    }

    template <MeshBufT... BufTs>
    void _set_from_mesh_group(const grx_cpu_mesh_group<BufTs...>& mesh_group) {
        _set_from_mesh_group(mesh_group, std::make_index_sequence<sizeof...(BufTs)>());
    }

    template <MeshBufT... BufTs, size_t... Idxs>
    void _set_from_mesh_group(const grx_cpu_mesh_group<BufTs...>& mesh_group, std::index_sequence<Idxs...>&&) {
        _elements = mesh_group.elements();
        ((_vbo.template set_data<Idxs>(mesh_group.template get<BufTs::tag>())), ...);
    }

    template <size_t... Idxs>
    MeshT to_mesh_group(std::index_sequence<Idxs...>&&) const {
        MeshT mesh_group;
        mesh_group.elements(_elements);
        ((mesh_group.template set<Idxs>(_vbo.template get_data<Idxs>())), ...);
        return mesh_group;
    }

private:
    grx_vbo_tuple<Ts...>           _vbo;
    core::vector<grx_mesh_element> _elements;

    enum unitag : size_t {
        unitag_mvp = 0,
        unitag_model,
        unitag_bone_matrices,
        unitag_texture0,
        unitag_texture1
    };
    grx_uniform_cache<core::optional<glm::mat4>,
                      core::optional<glm::mat4>,
                      core::optional<core::span<glm::mat4>>,
                      core::optional<int>,
                      core::optional<int>>
        _unicache{"MVP", "M", "bone_matrices", "texture0", "texture1"};

    core::vector<grx_texture_set<4>> _texture_sets;
};

template <MeshBufT... BufTs>
grx_object(const grx_cpu_mesh_group<BufTs...>& mesh_group)
    -> grx_object<false, grx_cpu_mesh_group<BufTs...>, typename BufTs::type...>;

template <MeshBufT... BufTs>
grx_object(const grx_cpu_mesh_group<BufTs...>& mesh_group, grx_object_instanced_tag&&)
    -> grx_object<true,
                  grx_cpu_mesh_group<BufTs...>,
                  typename BufTs::type...,
                  core::vector<glm::mat4>,
                  core::vector<glm::mat4>>;

namespace details {
    template <CpuMeshGroup MeshT, bool Instanced>
    struct object_type {
        using type = decltype(grx_object(std::declval<MeshT>(), grx_object_instanced_tag{}));
    };

    template <CpuMeshGroup MeshT>
    struct object_type<MeshT, false> {
        using type = decltype(grx_object(std::declval<MeshT>()));
    };

    template <CpuMeshGroup MeshT, bool Instanced>
    using object_type_t = typename object_type<MeshT, Instanced>::type;
}

template <CpuMeshGroup M, bool Instanced = false>
auto try_load_object(const core::shared_ptr<grx_texture_mgr<4>>& texture_mgr,
                     const core::string&                         relative_path)
    -> core::try_opt<details::object_type_t<M, Instanced>> {
    using ObjectT = details::object_type_t<M, Instanced>;
    using namespace core;

    core::cfg_path path{"models_dir", relative_path};
    auto absolute_path = path.absolute();
    auto relative_dir = core::path_eval(relative_path / "..");

    if (core::has_extension(absolute_path, ".dae")) {
        auto data = core::read_file(absolute_path);

        if (!data)
            return std::runtime_error("Can't load mesh at path '" + absolute_path + "'");

        try {
            auto str_data = grx_utils::collada_bake_bind_shape_matrix(*data);
            auto scene    = details::assimp_load_scene(*data);
            auto guard    = core::scope_guard{[&]() {
                details::assimp_release_scene(scene);
            }};

            auto mesh_group = M::from_assimp(scene);
            if constexpr (M::has_bone_buf()) {
                /* TODO: mesh can has no animations */
                auto skeleton           = grx_skeleton::from_assimp(scene);
                auto animations         = get_animations_optimized_from_assimp(scene, skeleton);
                auto skeleton_optimized = skeleton.get_optimized();

                auto object = ObjectT(mesh_group);
                object.set_skeleton(core::move(skeleton_optimized));
                object.set_animations(core::move(animations));
                object.set_textures(load_texture_set_from_paths(
                    texture_mgr,
                    get_texture_paths_from_assimp("models_dir", relative_dir, scene)));
                return object;
            }
            else {
                auto object = ObjectT(mesh_group);
                object.set_textures(load_texture_set_from_paths(
                    texture_mgr,
                    get_texture_paths_from_assimp("models_dir", relative_dir, scene)));
                return object;
            }
        }
        catch (const std::exception& e) {
            return std::runtime_error(e.what());
        }
    } else {
        ObjectT object;
        auto obj_data = core::read_binary_file(absolute_path);
        if (!obj_data)
            return std::runtime_error("Can't load object at path '" + absolute_path + "'");
        else {
            deserializer_view ds{*obj_data};
            ds.read(object);
            return object;
        }
    }

    return std::runtime_error("Can't load mesh object");
}

template <CpuMeshGroup M, bool Instanced = false>
auto load_object(const core::shared_ptr<grx_texture_mgr<4>>& texture_mgr,
                 const core::string&                         relative_path) {
    return core::move(try_load_object<M, Instanced>(texture_mgr, relative_path).value());
}

} // namespace grx

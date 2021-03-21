#include <core/resource_mgr_base.hpp>
#include "graphics/algorithms/grx_frustum_culling.hpp"
#include "graphics/grx_debug.hpp"
#include "grx_object.hpp"
#include "grx_movable.hpp"
#include "grx_animation_player.hpp"

namespace grx
{
template <bool IsInstanced, typename MeshT, typename... Ts>
class grx_object_mgr;


template <bool IsInstanced, typename MeshT, typename... Ts>
class grx_object_provider;

template <typename MeshT, typename... Ts>
class grx_object_provider<false, MeshT, Ts...>
    : public core::resource_provider_t<grx_object_mgr<false, MeshT, Ts...>>,
      public grx_movable,
      public grx_animation_player_if_has_skeleton<MeshT::has_bone_buf()>::type {
public:
    using super_t = core::resource_provider_t<grx_object_mgr<false, MeshT, Ts...>>;
    using super_t::super_t;
    using anim_player_t =
        typename grx_animation_player_if_has_skeleton<MeshT::has_bone_buf()>::type;

    template <bool HasSkeleton = MeshT::has_bone_buf()>
    std::enable_if_t<HasSkeleton> persistent_update(double framestep) {
        if constexpr (HasSkeleton) {

            auto* obj = this->try_access();
            bool visible = _aabb_proxy.is_visible(frustum_bits::csm_near |
                                                  frustum_bits::csm_middle | frustum_bits::csm_far);
            if (visible && obj)
                anim_player_t::persistent_anim_update(&obj->_animations, &obj->_skeleton, framestep);
            else
                anim_player_t::persistent_anim_update(nullptr, nullptr, framestep);
        }
    }

    template <typename ShaderT, bool HasSkeleton = MeshT::has_bone_buf()>
    void
    draw(const glm::mat4& view_projection, const ShaderT& program, bool enable_textures = true) {
        auto* obj = this->try_access();
        if (obj) {
            auto& model_mat = this->model_matrix();
            bool visible = _aabb_proxy.is_visible(frustum_bits::csm_near |
                                                  frustum_bits::csm_middle | frustum_bits::csm_far);
            if constexpr (!HasSkeleton)
                _aabb_proxy.aabb() = this->update_aabb(obj->aabb());
            else {
                /* TODO: only if not a ragdoll! */
                if constexpr (true) {
                    _aabb_proxy.aabb() = obj->overlap_aabb().get_transformed(model_mat);
                } else {
                    auto& final_transf = this->final_transforms().empty() ?
                        obj->_skeleton.final_transforms() : this->final_transforms();
                    _aabb_proxy.aabb() = obj->_skeleton.calc_aabb(model_mat, final_transf);
                }
            }

            if (!visible)
                return;

            if constexpr (!HasSkeleton) {
                obj->draw(view_projection, model_mat, program, enable_textures);
                grx_aabb_debug().push(obj->aabb(), model_mat, color_rgb{255, 0, 127});
            }
            else {
                if (this->final_transforms().empty()) {
                    obj->draw(view_projection, model_mat, program, enable_textures);
                    obj->_skeleton.debug_draw_aabbs(model_mat);
                } else {
                    obj->draw(view_projection,
                              model_mat,
                              program,
                              enable_textures,
                              this->final_transforms());
                    obj->_skeleton.debug_draw_aabbs(model_mat, this->final_transforms());
                }
                grx_aabb_debug().push(obj->overlap_aabb(), model_mat, color_rgb{255, 255, 255});
            }
        }
    }

private:
    grx_aabb_culling_proxy _aabb_proxy;
};

namespace details {
    struct instance_pair {
        grx_movable movable;
        grx_aabb_culling_proxy aabb_proxy;
    };

    struct instance_tuple {
        grx_movable movable;
        grx_animation_player anim_player;
        grx_aabb_culling_proxy aabb_proxy;
    };

    template <bool HasSkeleton>
    struct final_bone_transforms_storage {
        core::hash_map<core::u64, instance_pair> _instances;
    };

    template <>
    struct final_bone_transforms_storage<true> {
        core::hash_map<core::u64, instance_tuple> _instances;
        core::vector<glm::mat4>                   _all_final_transforms;
    };
}

template <typename MeshT, typename... Ts>
class grx_object_provider<true, MeshT, Ts...>
    : public core::resource_provider_t<grx_object_mgr<true, MeshT, Ts...>>,
      public details::final_bone_transforms_storage<MeshT::has_bone_buf()> {
public:
    using super_t = core::resource_provider_t<grx_object_mgr<true, MeshT, Ts...>>;
    using super_t::super_t;

    struct instance_pair {
        grx_movable movable;
        grx_aabb_culling_proxy aabb_proxy;
    };

    class instance {
    public:
        instance(core::u64 iid, grx_object_provider* iprovider):
            id(iid), provider(iprovider) {}

        instance(const instance&) = delete;
        instance& operator=(const instance&) = delete;

        instance(instance&& i) noexcept: id(i.id), provider(i.provider) {
            i.id = 0;
        }

        instance& operator=(instance&& i) noexcept {
            id = i.id;
            provider = i.provider;
            i.id = 0;
        }

        ~instance() {
            if (id)
                provider->remove_instance_id(id);
        }

        [[nodiscard]]
        grx_movable& movable() {
            return provider->_instances.at(id).movable;
        }

        [[nodiscard]]
        const grx_movable& movable() const {
            return provider->_instances.at(id).movable;
        }

        template <bool Enable = MeshT::has_bone_buf()>
        [[nodiscard]]
        std::enable_if_t<Enable, grx_animation_player&> animation_player() {
            return provider->_instances.at(id).anim_player;
        }

        template <bool Enable = MeshT::has_bone_buf()>
        [[nodiscard]] std::enable_if_t<Enable, const grx_animation_player&>
        animation_player() const {
            return provider->_instances.at(id).anim_player;
        }

        template <bool HasSkeleton = MeshT::has_bone_buf()>
        std::enable_if_t<HasSkeleton> persistent_update(double framestep) {
            auto* obj     = provider->try_access();
            bool  visible = provider->_instances.at(id).aabb_proxy.is_visible(
                frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far);
            if (visible && obj)
                animation_player().persistent_anim_update(
                    &obj->_animations, &obj->_skeleton, framestep);
            else
                animation_player().persistent_anim_update(nullptr, nullptr, framestep);
        }

    private:
        core::u64              id;
        grx_object_provider*   provider;
    };


    template <bool HasSkeleton = MeshT::has_bone_buf()>
    instance create_instance() {
        auto id = _instance_counter++;
        this->_instances[id];

        return instance(id, this);
    }

    template <bool HasSkeleton = MeshT::has_bone_buf()>
    void remove_instance_id(core::u64 id) {
        this->_instances.erase(id);
    }

    template <typename ShaderT, bool HasSkeleton = MeshT::has_bone_buf()>
    void draw(const glm::mat4& view_projection,
                                  const ShaderT&   program,
                                  bool enable_textures = true) {
        if (this->_instances.empty())
            return;

        auto* obj = this->try_access();
        if (obj) {
            _model_mats.clear();

            if constexpr (HasSkeleton) {
                this->_all_final_transforms.clear();
                size_t bones_count = 0;

                for (auto& [_, instance] : this->_instances) {
                    bool visible = instance.aabb_proxy.is_visible(
                        frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far);

                    auto& final_transf = instance.anim_player.final_transforms().empty() ?
                        obj->_skeleton.final_transforms() : instance.anim_player.final_transforms();

                    PeAssertF(bones_count == 0 || bones_count == final_transf.size(),
                              "bones_count({}) == final_transf.size()({})",
                              bones_count,
                              final_transf.size());
                    bones_count = final_transf.size();

                    /* TODO: only if not a ragdoll! */
                    if constexpr (true) {
                        instance.aabb_proxy.aabb() =
                            obj->overlap_aabb().get_transformed(instance.movable.model_matrix());
                    }
                    else {
                        instance.aabb_proxy.aabb() =
                            obj->_skeleton.calc_aabb(instance.movable.model_matrix(), final_transf);
                    }

                    if (visible) {
                        auto start_size = this->_all_final_transforms.size();
                        this->_all_final_transforms.resize(start_size + bones_count);
                        std::memcpy(this->_all_final_transforms.data() + start_size,
                                final_transf.data(), bones_count * sizeof(glm::mat4));
                        _model_mats.push_back(instance.movable.model_matrix());
                    }
                }

                if (grx_aabb_debug().is_enabled()) {
                    for (auto& [model_mat, i] : core::value_index_view(_model_mats)) {
                        obj->_skeleton.debug_draw_aabbs(
                            model_mat,
                            core::span<const glm::mat4>{this->_all_final_transforms}.subspan(
                                static_cast<ssize_t>(i * bones_count),
                                static_cast<ssize_t>(bones_count)));

                        grx_aabb_debug().push(
                            obj->overlap_aabb(), model_mat, color_rgb{255, 255, 255});
                    }
                }

                obj->draw(view_projection,
                          program,
                          _model_mats,
                          this->_all_final_transforms,
                          bones_count,
                          enable_textures);
            }
            else {
                for (auto& [_, instance] : this->_instances) {
                    bool visible = instance.aabb_proxy.is_visible(
                        frustum_bits::csm_near | frustum_bits::csm_middle | frustum_bits::csm_far);
                    instance.aabb_proxy.aabb() = instance.movable.update_aabb(obj->aabb());

                    if (visible)
                        _model_mats.push_back(instance.movable.model_matrix());
                }

                if (grx_aabb_debug().is_enabled())
                    for (auto& model_mat : _model_mats)
                        grx_aabb_debug().push(obj->aabb(), model_mat, color_rgb{255, 0, 127});

                obj->draw(view_projection, program, _model_mats, enable_textures);
            }
        }
    }

private:
    core::vector<glm::mat4> _model_mats;
    core::u64 _instance_counter = 1;
};

template <bool IsInstanced, typename MeshT, typename... Ts>
struct grx_cached_mesh_t : public details::skeleton_storage<MeshT::has_bone_buf(), false> {

    template <bool HasSkeleton = MeshT::has_bone_buf()>
    void serialize(core::vector<core::byte>& s) const {
        core::serialize_all(s, mesh, texture_path_sets, aabb, overlap_aabb);
        if constexpr (HasSkeleton)
            grx_object_skeleton::serialize(s);
    }

    template <bool HasSkeleton = MeshT::has_bone_buf()>
    void deserialize(core::span<const core::byte>& d) {
        core::deserialize_all(d, mesh, texture_path_sets, aabb, overlap_aabb);
        if constexpr (HasSkeleton)
            grx_object_skeleton::deserialize(d);
    }

    using mgr_t = grx_object_mgr<IsInstanced, MeshT, Ts...>;
    using gpu_t = grx_object<IsInstanced, MeshT, typename Ts::type...>;

    template <typename M = grx_cpu_mesh_group<Ts...>>
    static grx_cached_mesh_t from_object(gpu_t object) {
        grx_cached_mesh_t result;

        result.mesh = object.to_mesh_group();
        result.texture_path_sets = texture_sets_to_paths(object.texture_sets());
        result.aabb = object.aabb();
        result.overlap_aabb = object.overlap_aabb();

        if constexpr (M::has_bone_buf()) {
            result._skeleton = core::move(object._skeleton);
            result._animations = core::move(object._animations);
        }

        return result;
    }

    template <typename M = grx_cpu_mesh_group<Ts...>>
    gpu_t to_object() && {
        gpu_t object{mesh};
        object.set_textures(
            load_texture_sets_from_paths<object_texture_t::component_type,
                                         object_texture_t::channels_count()>(texture_path_sets));
        object.aabb()         = aabb;
        object.overlap_aabb() = overlap_aabb;

        if constexpr (M::has_bone_buf()) {
            object._skeleton = core::move(this->_skeleton);
            object._animations = core::move(this->_animations);
        }
        return object;
    }

    template <typename M = grx_cpu_mesh_group<Ts...>>
    static grx_cached_mesh_t
    load_async(const core::shared_ptr<mgr_t>& mgr, const core::cfg_path& path) {
        using core::operator/;

        grx_cached_mesh_t cached;

        auto absolute_path = path.absolute();
        auto relative_dir = core::path_eval(path.path / "..");

        if (core::has_extension(absolute_path, ".dae")) {
            auto data = core::try_read_file(absolute_path);

            if (!data)
                pe_throw std::runtime_error("Can't load mesh at path '" + absolute_path + "'");

            auto str_data = grx_utils::collada_bake_bind_shape_matrix(*data);
            auto scene    = details::assimp_load_scene(str_data);
            auto guard    = core::scope_guard{[&]() {
                details::assimp_release_scene(scene);
            }};

            auto mesh_group = M::from_assimp(scene);
            for (auto& element : mesh_group.elements())
                cached.aabb.merge(element.aabb);

            if constexpr (M::has_bone_buf()) {
                /* TODO: mesh can has no animations */
                auto skeleton           = grx_skeleton::from_assimp(scene);
                auto animations         = get_animations_optimized_from_assimp(scene, skeleton);
                auto skeleton_optimized = skeleton.get_optimized();

                cached.mesh = core::move(mesh_group);
                cached.set_skeleton(core::move(skeleton_optimized));
                cached.set_animations(core::move(animations));
                cached.overlap_aabb = cached.calc_overlap_aabb(cached.aabb);
                cached.texture_path_sets = get_texture_paths_from_assimp(
                    "models_dir", relative_dir, scene, mgr->texture_mgr().mgr_tag());
            }
            else {
                cached.mesh              = core::move(mesh_group);
                cached.texture_path_sets = get_texture_paths_from_assimp(
                    "models_dir", relative_dir, scene, mgr->texture_mgr().mgr_tag());
            }
        }
        else {
            auto bytes = core::try_read_binary_file(absolute_path);
            if (!bytes)
                pe_throw std::runtime_error("Can't open object at path '" + absolute_path + "'");

            auto ds = core::deserializer_view(*bytes);
            ds.read(cached);
        }

        return cached;
    }

    grx_cpu_mesh_group<Ts...> mesh;
    core::vector<grx_texture_path_set> texture_path_sets;
    grx_aabb                           aabb = grx_aabb::maximized();
    grx_aabb                           overlap_aabb = grx_aabb::maximized();
};

template <bool IsInstanced, typename MeshT, typename... Ts>
class grx_object_mgr
    : public core::resource_mgr_base<grx_cached_mesh_t<IsInstanced, MeshT, Ts...>,
                                     grx_object<IsInstanced, MeshT, typename Ts::type...>,
                                     grx_object_mgr<IsInstanced, MeshT, Ts...>,
                                     grx_object_provider<IsInstanced, MeshT, Ts...>> {
public:
    using gpu_t    = grx_object<IsInstanced, MeshT, typename Ts::type...>;
    using cached_t = grx_cached_mesh_t<IsInstanced, MeshT, Ts...>;

    using super_t = core::resource_mgr_base<cached_t,
                                            gpu_t,
                                            grx_object_mgr<IsInstanced, MeshT, Ts...>,
                                            grx_object_provider<IsInstanced, MeshT, Ts...>>;

    using super_t::super_t;

    static core::shared_ptr<grx_object_mgr> create_shared(const core::string& mgr_tag) {
        auto result = super_t::create_shared(mgr_tag);

        if (auto txtr_mgr = object_texture_mgr_t::global_mgr_weak_ptr().try_get(mgr_tag + "_txtr"))
            if (auto txtr_m = txtr_mgr->lock())
                result->_texture_mgr = core::move(txtr_m);

        if (!result->_texture_mgr)
            result->_texture_mgr = object_texture_mgr_t::create_shared(mgr_tag + "_txtr");
        return result;
    }

    auto load_async_cached(const core::cfg_path& path) {
        DLOG("resource_mgr[{}]: async load object {}", this->mgr_tag(), path);
        return core::submit_job(
            [this, path]() { return cached_t::load_async(this->shared_from_this(), path); });
    }

    static cached_t to_cache(gpu_t object) {
        return cached_t::from_object(move(object));
    }

    static gpu_t from_cache(cached_t cached) {
        return core::move(cached).to_object();
    }

    [[nodiscard]]
    const object_texture_mgr_t& texture_mgr() const {
        return *_texture_mgr;
    }

    grx_object_mgr(typename core::constructor_accessor<grx_object_mgr>::cref,
                   const core::string& imgr_tag):
        super_t(core::constructor_accessor<super_t>{}, imgr_tag),
        _texture_mgr(object_texture_mgr_t::create_shared(imgr_tag + "_txtr")) {}

private:
    core::shared_ptr<object_texture_mgr_t> _texture_mgr;
};

template <bool IsInstanced, typename MeshT>
struct grx_object_type_to_mgr{};

template <bool IsInstanced, typename... Ts>
struct grx_object_type_to_mgr<IsInstanced, grx_cpu_mesh_group<Ts...>> {
    using type = grx_object_mgr<IsInstanced, grx_cpu_mesh_group<Ts...>, Ts...>;
};

template <bool IsInstanced, typename MeshT>
using grx_object_type_to_mgr_t = typename grx_object_type_to_mgr<IsInstanced, MeshT>::type;

/*
class grx_object_mgr : public std::enable_shared_from_this<grx_object_mgr<IsInstanced, MeshT, Ts...>> {
public:
    using object_id_t = core::u64;

    struct value_t {
        core::optional<cached_mesh_t> cached;
        core::optional<grx_object<IsInstanced, MeshT, Ts...>> object;
    };

    struct object_spec_t {
        core::cfg_path        path;
        core::u32             usages;
        grx_load_significance load_significance;
    };

    static core::shared_ptr<grx_object_mgr> create_shared(const core::string& mgr_tag) {
        auto ptr = core::make_shared<grx_object_mgr>(core::constructor_accessor<grx_object_mgr>{}, mgr_tag);
    }

private:
    grx_object_mgr(typename core::constructor_accessor<grx_texture_mgr<S>>::cref, const core::string& imgr_tag) {
        _mgr_tag = imgr_tag;
    }

private:
    core::string _mgr_tag;
    texture_id_t _last_id = 0;
};
*/

} // namespace grx

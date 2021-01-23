#include <core/resource_mgr_base.hpp>
#include "grx_object.hpp"

namespace grx
{
template <bool IsInstanced, typename MeshT, typename... Ts>
class grx_object_mgr;


template <bool IsInstanced, typename MeshT, typename... Ts>
class grx_object_provider;

template <typename MeshT, typename... Ts>
class grx_object_provider<false, MeshT, Ts...>
    : public core::resource_provider_t<grx_object_mgr<false, MeshT, Ts...>> {
public:
    using super_t = core::resource_provider_t<grx_object_mgr<false, MeshT, Ts...>>;
    using super_t::super_t;

    struct anim_spec_t {
        core::string name;
        core::timer  timer;
        bool         stop_at_end = true;
    };

    template <bool Enable = MeshT::has_bone_buf()>
    std::enable_if_t<Enable> play_animation(const core::string& name, bool stop_at_end = true) {
        current_anim = anim_spec_t{name, {}, stop_at_end};
    }

    template <typename ShaderT, bool Enable = MeshT::has_bone_buf()>
    std::enable_if_t<Enable> draw(const glm::mat4& view_projection,
                                  const ShaderT&   program,
                                  bool             enable_textures = true) {
        auto* obj = this->try_access();
        if (obj) {
            if (current_anim) {
                auto& anim = *current_anim;
                auto& animations = obj->_animations;
                auto anim_pos = animations.find(anim.name);

                if (anim_pos == animations.end()) {
                    LOG_ERROR("Animation {} not found!", anim.name);
                    current_anim.reset();
                    obj->draw(view_projection, _model_mat, program, enable_textures);
                } else {
                    obj->draw(view_projection, _model_mat, program, enable_textures,
                            obj->_skeleton.animation_transforms(anim_pos->second, anim.timer.measure_count()));
                }
            } else {
                obj->draw(view_projection, _model_mat, program, enable_textures);
            }
        }
    }

private:
    core::optional<anim_spec_t> current_anim;
    glm::mat4                   _model_mat = glm::mat4(1.f);
};

template <typename MeshT, typename... Ts>
class grx_object_provider<true, MeshT, Ts...>
    : public core::resource_provider_t<grx_object_mgr<true, MeshT, Ts...>> {
public:
    using super_t = core::resource_provider_t<grx_object_mgr<true, MeshT, Ts...>>;
    using super_t::super_t;
};

template <bool IsInstanced, typename MeshT, typename... Ts>
struct grx_cached_mesh_t : public details::skeleton_storage<MeshT::has_bone_buf()> {

    template <bool HasSkeleton = MeshT::has_bone_buf()>
    void serialize(core::vector<core::byte>& s) const {
        core::serialize_all(s, mesh, texture_path_sets);
        if constexpr (HasSkeleton)
            grx_object_skeleton::serialize(s);
    }

    template <bool HasSkeleton = MeshT::has_bone_buf()>
    void deserialize(core::span<const core::byte>& d) {
        core::deserialize_all(d, mesh, texture_path_sets);
        if constexpr (HasSkeleton)
            grx_object_skeleton::deserialize(d);
    }

    using mgr_t = grx_object_mgr<IsInstanced, MeshT, Ts...>;
    using gpu_t = decltype(load_object<grx_cpu_boned_mesh_group_t>(
        grx_texture_mgr<OBJECT_TEXTURE_CHANS>::create_shared(""), ""));


    template <typename M = grx_cpu_mesh_group<Ts...>>
    static grx_cached_mesh_t from_object(gpu_t object) {
        grx_cached_mesh_t result;

        result.mesh = object.to_mesh_group();
        result.texture_path_sets = texture_sets_to_paths(object.texture_sets());

        if constexpr (M::has_bone_buf()) {
            result._skeleton = core::move(object._skeleton);
            result._animations = core::move(object._animations);
        }

        return result;
    }

    template <typename M = grx_cpu_mesh_group<Ts...>>
    gpu_t to_object() && {
        gpu_t object{mesh};
        object.set_textures(load_texture_sets_from_paths<OBJECT_TEXTURE_CHANS>(texture_path_sets));

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
            if constexpr (M::has_bone_buf()) {
                /* TODO: mesh can has no animations */
                auto skeleton           = grx_skeleton::from_assimp(scene);
                auto animations         = get_animations_optimized_from_assimp(scene, skeleton);
                auto skeleton_optimized = skeleton.get_optimized();

                cached.mesh = core::move(mesh_group);
                cached.set_skeleton(core::move(skeleton_optimized));
                cached.set_animations(core::move(animations));
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
};

template <bool IsInstanced, typename MeshT, typename... Ts>
class grx_object_mgr
    : public core::resource_mgr_base<grx_cached_mesh_t<IsInstanced, MeshT, Ts...>,
                                     decltype(load_object<grx_cpu_boned_mesh_group_t>(
                                         grx_texture_mgr<OBJECT_TEXTURE_CHANS>::create_shared(""),
                                         "")),
                                     grx_object_mgr<IsInstanced, MeshT, Ts...>,
                                     grx_object_provider<IsInstanced, MeshT, Ts...>> {
public:
    using gpu_t    = decltype(load_object<grx_cpu_boned_mesh_group_t>(
        grx_texture_mgr<OBJECT_TEXTURE_CHANS>::create_shared(""), ""));
    using cached_t = grx_cached_mesh_t<IsInstanced, MeshT, Ts...>;

    using super_t = core::resource_mgr_base<cached_t,
                                            gpu_t,
                                            grx_object_mgr<IsInstanced, MeshT, Ts...>,
                                            grx_object_provider<IsInstanced, MeshT, Ts...>>;

    using super_t::super_t;

    static core::shared_ptr<grx_object_mgr> create_shared(const core::string& mgr_tag) {
        auto result = super_t::create_shared(mgr_tag);
        result->_texture_mgr = grx_texture_mgr<OBJECT_TEXTURE_CHANS>::create_shared(mgr_tag + "_txtr");
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
    const grx_texture_mgr<OBJECT_TEXTURE_CHANS>& texture_mgr() const {
        return *_texture_mgr;
    }

    grx_object_mgr(typename core::constructor_accessor<grx_object_mgr>::cref,
                   const string& imgr_tag):
        super_t(core::constructor_accessor<super_t>{}, imgr_tag),
        _texture_mgr(grx_texture_mgr<OBJECT_TEXTURE_CHANS>::create_shared(imgr_tag + "_txtr")) {}

private:
    core::shared_ptr<grx_texture_mgr<OBJECT_TEXTURE_CHANS>> _texture_mgr;
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

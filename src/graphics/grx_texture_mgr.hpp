#pragma once

#include "graphics/grx_types.hpp"
#include "grx_texture.hpp"

#include <core/serialization.hpp>
#include <core/config_manager.hpp>
#include <core/global_storage.hpp>

namespace grx {

template <size_t S>
class grx_texture_mgr;

namespace details {
    template <size_t S>
    using texture_mgr_lookup_t = core::global_storage<core::string, core::weak_ptr<grx_texture_mgr<S>>>;

    template <size_t S>
    texture_mgr_lookup_t<S>& texture_mgr_lookup() {
        return texture_mgr_lookup_t<S>::instance();
    }
}

template <size_t S>
class grx_texture_id {
public:
    void serialize(core::vector<core::byte>& s) const;
    void deserialize(core::span<const core::byte>& d);

    grx_texture_id() = default;

    grx_texture_id(core::shared_ptr<grx_texture_mgr<S>> texture_mgr);

    grx_texture_id(core::shared_ptr<grx_texture_mgr<S>> texture_mgr,
                   const core::cfg_path&                file_path,
                   grx_load_significance load_significance = grx_load_significance::medium);
    ~grx_texture_id();

    grx_texture_id(const grx_texture_id&);
    grx_texture_id& operator=(const grx_texture_id&);

    grx_texture_id(grx_texture_id&&) noexcept;
    grx_texture_id& operator=(grx_texture_id&&) noexcept;

    /**
     * @brief Bind texture. Equivalent of glBindTexture
     */
    void bind() const {
        wait_for_id();
        grx_texture_helper::bind_texture(_cached_id);
    }

    /**
     * @brief Activate texture
     *
     * Equivalent of glActiveTexture
     *
     * @param number - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
     */
    void activate(uint number) const {
        Expects(number < 32);
        wait_for_id();
        grx_texture_helper::active_texture(number);
    }

    /**
     * @brief Activate texture and bind
     *
     * Equivalent of glActiveTexture and glBindTexture calls (or glBindTextureUnit)
     *
     * @param number - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
     */
    void bind_unit(uint number) const {
        Expects(number < 32);
        wait_for_id();
        grx_texture_helper::bind_unit(_cached_id, number);
    }

    void load_significance(grx_load_significance load_significance);
    grx_load_significance load_significance() const;

    [[nodiscard]]
    core::u32 usages() const;

    [[nodiscard]]
    const core::cfg_path& path() const;

    [[nodiscard]]
    grx_texture<S>* try_access() const;

private:
    inline void wait_for_id() const;

    core::shared_ptr<grx_texture_mgr<S>> _storage;
    core::u64                            _texture_id = core::numlim<core::u64>::max();
    mutable uint                         _cached_id = core::numlim<uint>::max();
};

template <size_t S>
class grx_texture_mgr : public std::enable_shared_from_this<grx_texture_mgr<S>> {
public:
    using texture_id_t = core::u64;

    struct texture_value_t {
        core::optional<grx_color_map<core::u8, S>> image;
        core::optional<grx_texture<S>>             texture;
    };

    struct texture_spec_t {
        core::cfg_path        path;
        core::u32             usages;
        grx_load_significance load_significance;
    };

    static core::shared_ptr<grx_texture_mgr> create_shared(const core::string& mgr_tag) {
        auto ptr = core::make_shared<grx_texture_mgr>(core::constructor_accessor<grx_texture_mgr>{}, mgr_tag);
        details::texture_mgr_lookup<S>().insert(mgr_tag, ptr);
        return ptr;
    }

    texture_id_t load_id(const core::cfg_path& path, grx_load_significance load_significance = grx_load_significance::medium) {
        using namespace core;

        auto [position, was_inserted] = _path_to_id.emplace(path.absolute(), 0);
        if (!was_inserted) {
            auto& spec = _specs[position->second];
            ++spec.usages;
            spec.load_significance = load_significance;
            return position->second;
        }

        auto id = new_id();
        position->second = id;
        _specs[position->second] = texture_spec_t{path, 1, load_significance};
        _futures.emplace(id, load_color_map_async<core::vec<core::u8, S>>(path.absolute()));

        return id;
    }

    grx_texture_id<S> load(const core::cfg_path& path, grx_load_significance load_significance = grx_load_significance::medium) {
        return grx_texture_id<S>(this->shared_from_this(), path, load_significance);
    }

    void increment_usages(texture_id_t id) {
        auto found_spec = _specs.find(id);

        if (found_spec == _specs.end())
            throw std::runtime_error("Texture with specified id was not found");

        auto& spec = found_spec->second;
        ++spec.usages;

        if (spec.usages == 1)
            load_to_video_memory(id);
    }

    void decrement_usages(texture_id_t id) {
        auto found_spec = _specs.find(id);

        if (found_spec == _specs.end())
            throw std::runtime_error("Texture with specified id was not found");

        auto& spec = found_spec->second;
        if (spec.usages == 0)
            return;

        --spec.usages;
        if (spec.usages == 0) {
            auto found_texture = _textures.find(id);
            assert(found_texture != _textures.end());
            auto& tex = found_texture->second;

            if (spec.load_significance == grx_load_significance::medium) {
                tex.image = tex.texture.value().to_color_map();
                tex.texture = core::nullopt;
            } else if (spec.load_significance == grx_load_significance::low)
                tex.texture = core::nullopt;
        }
    }

    grx_texture<S>* try_access(texture_id_t id, bool wait = false) {
        auto found_texture = _textures.find(id);
        if (found_texture != _textures.end())
            return &found_texture->second.texture.value();

        auto found_future = _futures.find(id);
        if (found_future == _futures.end())
            return nullptr;

        auto found_spec = _specs.find(id);
        if (found_spec == _specs.end())
            return nullptr;

        auto& future = found_future->second;
        auto& spec   = found_spec->second;

        if (wait || future / core::is_ready()) {
            texture_value_t texture;
            if (spec.usages)
                texture.texture = grx_texture<S>(future.get().value());
            else
                texture.image = future.get().value();

            _futures.erase(found_future);

            auto [position, _] = _textures.insert_or_assign(id, core::move(texture));
            if (position->second.texture)
                return &(*position->second.texture);
        }

        return nullptr;
    }

    grx_texture<S>& access(texture_id_t id) {
        auto texture = try_access(id, true);
        if (texture == nullptr)
            throw std::runtime_error("Texture with specified id was not found");
        return *texture;
    }

    [[nodiscard]]
    grx_load_significance load_significance(texture_id_t id) const {
        return _specs.at(id).load_significance;
    }

    void load_significance(texture_id_t id, grx_load_significance load_significance) {
        _specs.at(id).load_significance = load_significance;
    }

    [[nodiscard]]
    const core::cfg_path& file_path(texture_id_t id) const {
        return _specs.at(id).path;
    }

    [[nodiscard]]
    core::u32 usages(texture_id_t id) const {
        return _specs.at(id).usages;
    }

    grx_texture_mgr(typename core::constructor_accessor<grx_texture_mgr<S>>::cref, const core::string& imgr_tag) {
        _mgr_tag = imgr_tag;
    }

    ~grx_texture_mgr() noexcept {
        details::texture_mgr_lookup<S>().remove(_mgr_tag);
    }

    grx_texture_mgr(grx_texture_mgr&&) noexcept = default;
    grx_texture_mgr& operator=(grx_texture_mgr&&) noexcept = default;
    grx_texture_mgr(const grx_texture_mgr&) noexcept = delete;
    grx_texture_mgr& operator=(const grx_texture_mgr&) noexcept = delete;

    [[nodiscard]]
    const core::string& mgr_tag() const {
        return _mgr_tag;
    }

private:
    friend struct core::constructor_accessor<grx_texture_mgr>;


    texture_id_t new_id() {
        return _last_id++;
    }

    void load_to_video_memory(texture_id_t id) {
        auto found_texture = _textures.find(id);
        if (found_texture == _textures.end())
            return;

        auto& texture = found_texture->second;
        if (!texture.texture) {
            if (texture.image) {
                texture.texture = grx_texture<S>(*texture.image);
                texture.image = core::nullopt;
            } else {
                auto found_spec = _specs.find(id);
                assert(found_spec != _specs.end());
                _futures.emplace(id, load_color_map_async<core::vec<core::u8, S>>(found_spec->second.path.absolute()));
            }
        }
    }

private:
    core::hash_map<texture_id_t, core::job_future<core::try_opt<grx_color_map<core::u8, S>>>> _futures;
    core::hash_map<texture_id_t, texture_value_t> _textures;
    core::hash_map<texture_id_t, texture_spec_t> _specs;
    core::hash_map<core::string, texture_id_t>   _path_to_id;
    core::string _mgr_tag;
    texture_id_t _last_id = 0;
};

template <size_t S>
void grx_texture_id<S>::serialize(core::vector<core::byte>& s) const {
    auto load_sign = static_cast<std::underlying_type_t<grx_load_significance>>(load_significance());
    core::serialize_all(s, _storage->mgr_tag(), path(), load_sign);
}

template <size_t S>
void grx_texture_id<S>::deserialize(core::span<const core::byte>& d) {
    using core::operator/;

    core::string          mgr_tag;
    core::cfg_path        filepath;
    std::underlying_type_t<grx_load_significance> load_sign;
    core::deserialize_all(d, mgr_tag, filepath, load_sign);

    auto mgr = details::texture_mgr_lookup<S>().get(mgr_tag).lock();
    if (!mgr)
        throw std::runtime_error("Can't find texture manager at tag '" + mgr_tag + "'");

    *this = grx_texture_id<S>(move(mgr), filepath, static_cast<grx_load_significance>(load_sign));
}

template <size_t S>
grx_texture_id<S>::grx_texture_id(core::shared_ptr<grx_texture_mgr<S>> texture_mgr,
                                  const core::cfg_path&                file_path,
                                  grx_load_significance                load_significance):
    _storage(core::move(texture_mgr)) {
    _texture_id = _storage->load_id(file_path, load_significance);
}

template <size_t S>
grx_texture_id<S>::~grx_texture_id() {
    if (_texture_id != core::numlim<core::u64>::max()) {
        assert(_storage);
        _storage->decrement_usages(_texture_id);
    }
}

template <size_t S>
grx_texture_id<S>::grx_texture_id(const grx_texture_id<S>& texture) {


    _storage = texture._storage;
    _texture_id = texture._texture_id;
    _cached_id = texture._cached_id;
    _storage->increment_usages(_texture_id);
}

template <size_t S>
grx_texture_id<S>& grx_texture_id<S>::operator=(const grx_texture_id<S>& texture) {
    _storage = texture._storage;
    _texture_id = texture._texture_id;
    _cached_id = texture._cached_id;
    _storage->increment_usages(_texture_id);

    return *this;
}

template <size_t S>
grx_texture_id<S>::grx_texture_id(grx_texture_id<S>&& texture) noexcept {
    _storage    = core::move(texture._storage);
    _texture_id = texture._texture_id;
    _cached_id  = texture._cached_id;

    texture._texture_id = core::numlim<core::u64>::max();
    texture._cached_id  = core::numlim<uint>::max();
}

template <size_t S>
grx_texture_id<S>& grx_texture_id<S>::operator=(grx_texture_id<S>&& texture) noexcept {
    _storage    = core::move(texture._storage);
    _texture_id = texture._texture_id;
    _cached_id  = texture._cached_id;

    texture._texture_id = core::numlim<core::u64>::max();
    texture._cached_id  = core::numlim<uint>::max();

    return *this;
}

template <size_t S>
void grx_texture_id<S>::load_significance(grx_load_significance load_significance) {
    _storage->load_significance(_texture_id, load_significance);
}

template <size_t S>
grx_load_significance grx_texture_id<S>::load_significance() const {
    return _storage->load_significance(_texture_id);
}

template <size_t S>
core::u32 grx_texture_id<S>::usages() const {
    return _storage->usages(_texture_id);
}

template <size_t S>
const core::cfg_path& grx_texture_id<S>::path() const {
    return _storage->file_path(_texture_id);
}

template <size_t S>
void grx_texture_id<S>::wait_for_id() const {
    _cached_id = _storage->access(_texture_id).raw_id();
}

template <size_t S>
grx_texture<S>* grx_texture_id<S>::try_access() const {
    return _storage->try_access(_texture_id);
}
} // namespace grx

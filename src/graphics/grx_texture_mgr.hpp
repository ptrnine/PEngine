#pragma once

#include "graphics/grx_types.hpp"
#include "grx_texture.hpp"

#include <core/serialization.hpp>
#include <core/config_manager.hpp>
#include <core/resource_mgr_base.hpp>

namespace grx {

template <ColorComponent T, size_t S>
class grx_texture_mgr;


template <ColorComponent T, size_t S>
class grx_texture_provider : public core::resource_provider_t<grx_texture_mgr<T, S>> {
public:
    using super_t = core::resource_provider_t<grx_texture_mgr<T, S>>;

    grx_texture_provider() = default;
    ~grx_texture_provider() = default;

    grx_texture_provider(
        core::shared_ptr<grx_texture_mgr<T, S>> mgr,
        const core::cfg_path&                file_path,
        core::load_significance_t            load_significance = core::load_significance_t::medium):
        super_t(core::move(mgr), file_path, load_significance) {}

    grx_texture_provider(const grx_texture_provider& resource):
        super_t(resource), _cached_id(resource._cached_id) {}

    grx_texture_provider& operator=(const grx_texture_provider& resource) {
        super_t::operator=(resource);
        _cached_id = resource._cached_id;

        return *this;
    }

    grx_texture_provider(grx_texture_provider&& resource) noexcept:
        super_t(core::move(resource)), _cached_id(resource._cached_id) {
        resource._cached_id = core::numlim<uint>::max();
    }

    grx_texture_provider& operator=(grx_texture_provider&& resource) noexcept {
        super_t::operator=(core::move(resource));
        _cached_id = resource._cached_id;

        resource._cached_id = core::numlim<uint>::max();

        return *this;
    }
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
private:
    void wait_for_id() const {
        if (_cached_id == core::numlim<uint>::max())
            _cached_id = this->_storage->access(this->_resource_id).raw_id();
    }

    uint _cached_id = core::numlim<uint>::max();
};


template <ColorComponent T, size_t S>
class grx_texture_mgr : public core::resource_mgr_base<grx_color_map<T, S>,
                                                       grx_texture<T, S>,
                                                       grx_texture_mgr<T, S>,
                                                       grx_texture_provider<T, S>> {
public:
    using core::resource_mgr_base<grx_color_map<T, S>,
                                  grx_texture<T, S>,
                                  grx_texture_mgr<T, S>,
                                  grx_texture_provider<T, S>>::resource_mgr_base;

    auto load_async_cached(const core::cfg_path& path) {
        DLOG("resource_mgr[{}]: async load texture {}", this->mgr_tag(), path);
        return load_color_map_async<core::vec<core::u8, S>>(path.absolute());
    }

    static grx_color_map<T, S> to_cache(grx_texture<T, S> texture) {
        return texture.to_color_map();
    }

    static grx_texture<T, S> from_cache(grx_color_map<T, S>&& color_map) {
        return grx_texture(color_map);
    }
};
} // namespace grx

#pragma once

#include "core/async.hpp"
#include "grx_texture.hpp"

namespace core {
    class config_manager;
}

namespace grx {

    template <size_t S>
    class grx_texture_id;

    template <size_t, bool>
    class grx_texture_id_future;

    /**
     * @brief Load significance for textures
     */
    enum class texture_load_significance {
        low,    ///< kill all texture data if it has no usages
        medium, ///< unload texture to grx_color_map if it has no usages
        high    ///< never unload texture
    };


    /**
     * @brief Texture Manager
     *
     * Cached texture loading and little texture memory management
     *
     * Note: instance of this class can be created by static function grx_texture_mgr::create_shared
     */
    class grx_texture_mgr : public std::enable_shared_from_this<grx_texture_mgr> {
    public:
        using raw_id_t = uint;
        using path_t   = core::string;

        template <size_t S>
        struct path_val_t {
            raw_id_t id;
            core::optional<grx_color_map<uint8_t, S>> cached_color_map;
        };

        template <size_t S>
        struct texture_val_t {
            core::string                   path;
            core::optional<grx_texture<S>> texture;
            uint                           usages = 0;
            texture_load_significance      load_significance = texture_load_significance::medium;
        };

    public:
        grx_texture_mgr(const grx_texture_mgr&) = delete;
        grx_texture_mgr(grx_texture_mgr&&) noexcept = default;

        static core::shared_ptr<grx_texture_mgr>
        create_shared(const core::config_manager& config_manager) {
            return core::shared_ptr<grx_texture_mgr>(new grx_texture_mgr(config_manager));
        }

        /**
         * @brief load texture from file with cache lookup
         *
         * Throws std::runtime_error if image file not contain valid image
         * or file not found
         *
         * @tparam T - color type
         * @param file_path - path to image file
         *
         * @return result texture_id
         */
        template <core::MathVector T>
        grx_texture_id<T::size()> load_unwrap(core::string_view file_path) {
            auto path = core::path_eval(file_path);
            return try_load<T::size(), true>(path);
        }

        /**
         * @brief Load texture from file with cache lookup
         *
         * Throws std::runtime_error if image file not contain valid image
         *
         * @tparam T - color type
         * @param file_path - path to image file
         *
         * @return result texture_id if on success or exception
         */
        template <core::MathVector T>
        core::try_opt<grx_texture_id<T::size()>>
        load(core::string_view file_path) {
            auto path = core::path_eval(file_path);
            return try_load<T::size(), false>(path);
        }

        /**
         * @brief Load texture from file asynchronously with cache lookup
         *
         * @throw std::runtime_error if image file not contain valid image or file not found
         *
         * @tparam T - color type
         * @param file_path - path to image file
         *
         * @return future to texture_id
         */
        template <core::MathVector T>
        grx_texture_id_future<T::size(), false>
        load_async_unwrap(core::string_view file_path) {
            return load_async_tmpl<T, false>(file_path);
        }

        /**
         * @brief Load texture from file asynchronously with cache lookup
         *
         * Throws std::runtime_error if image file not contain valid image
         *
         * @tparam T - color type
         * @param file_path - path to image file
         *
         * @return future to texture_id on success or future with exception
         */
        template <core::MathVector T>
        grx_texture_id_future<T::size(), true>
        load_async(core::string_view file_path) {
            return load_async_tmpl<T, true>(file_path);
        }

    private:
        template <core::MathVector T, bool Optional>
        grx_texture_id_future<T::size(), Optional>
        load_async_tmpl(core::string_view texture_path) {
            auto path = core::path_eval(texture_path);

            auto& map = path_to_id_map<T::size()>();
            auto  path_data_pos = map.find(path);

            if (path_data_pos == map.end()) {
                if constexpr (Optional)
                    return grx_texture_id_future<T::size(), Optional>(
                            load_color_map_async<T>(path), weak_from_this(), std::move(path));
                else
                    return grx_texture_id_future<T::size(), Optional>(
                            load_color_map_async_unwrap<T>(path), weak_from_this(), std::move(path));
            } else {
                auto& cached   = path_data_pos->second.cached_color_map;
                auto& data_map = id_to_data_map<T::size()>();
                auto&  id      = path_data_pos->second.id;

                if (cached) {
                    auto texture = grx_texture<T::size()>(*cached);
                    id = texture.raw_id();
                    data_map[id].texture = std::move(texture);
                }
                data_map[id].usages++;

                return grx_texture_id_future<T::size(), Optional>(
                        grx_texture_id<T::size()>(id, weak_from_this()));
            }
        }

    protected:
        template <size_t S>
        friend class grx_texture_id;

        template <size_t, bool>
        friend class grx_texture_id_future;

        template <size_t S>
        void increment_usages(raw_id_t id) {
            id_to_data_map<S>()[id].usages++;
        }

        template <size_t S>
        void decrement_usages(raw_id_t id) {
            auto& data = id_to_data_map<S>()[id];
            data.usages--;

            /*
             * If no usages and not high significance - unload
             */
            if (data.usages == 0 && data.load_significance != texture_load_significance::high) {
                if (data.texture) {
                    /*
                     * If medium significance - unload texture to color_map (to RAM)
                     */
                    if (data.load_significance == texture_load_significance::medium) {
                        auto& id_and_cache = path_to_id_map<S>()[data.path];
                        id_and_cache.cached_color_map = data.texture->to_color_map();
                    }

                    data.texture.reset();
                } else {
                    LOG_WARNING("Can't unload texture '{}': texture not loaded!", data.path);
                }

                /*
                 * If low significance - fully cleanup
                 */
                if (data.load_significance == texture_load_significance::low)
                    path_to_id_map<S>().erase(data.path);

                id_to_data_map<S>().erase(id);
            }
        }

    private:
        grx_texture_mgr(const core::config_manager& config_manager);

        template <size_t S, bool Unwrap>
        auto try_load(const path_t& path) {
            auto& map = path_to_id_map<S>();
            auto [position, was_inserted] = map.emplace(path, path_val_t<S>());

            /*
             * If path already in map, but have no associated texture or cached image then reinserting
             */
            if (!was_inserted) {
                auto& texture_opt = id_to_data_map<S>()[position->second.id].texture;
                if (!texture_opt && !position->second.cached_color_map)
                    was_inserted = true;
            }

            if (was_inserted) {
                if constexpr (Unwrap) {
                    auto  texture  = load_texture_unwrap<core::vec<uint8_t, S>>(path);
                    auto  raw_id   = texture.raw_id();
                    auto& data_map = id_to_data_map<S>();
                    auto& data     = data_map[raw_id];

                    position->second.id = raw_id;
                    data.texture = std::move(texture);
                    data.path    = path;
                    data.usages  = 1;
                }
                else {
                    auto texture_opt = load_texture<core::vec<uint8_t, S>>(path);

                    if (!texture_opt)
                        return core::try_opt<grx_texture_id<S>>(texture_opt.exception_ptr());

                    auto  raw_id   = texture_opt->raw_id();
                    auto& data_map = id_to_data_map<S>();
                    auto& data     = data_map[raw_id];

                    position->second.id = raw_id;
                    data.texture = std::move(*texture_opt);
                    data.path    = path;
                    data.usages  = 1;
                }

                return grx_texture_id<S>(position->second.id, weak_from_this());
            }
            else {
                auto  raw_id      = position->second.id;
                auto& data_map    = id_to_data_map<S>();
                auto& data        = data_map[raw_id];
                auto& texture_opt = data.texture;
                auto& cached_opt  = position->second.cached_color_map;

                data.usages++;

                if (!texture_opt) {
                    texture_opt = *cached_opt;
                    cached_opt.reset();
                }

                return grx_texture_id<S>(raw_id, weak_from_this());
            }
        }

        template <size_t S>
        core::hash_map<path_t, path_val_t<S>>& path_to_id_map() {
            return std::get<S - 1>(_path_to_id_map);
        }

        template <size_t S>
        core::hash_map<raw_id_t, texture_val_t<S>>& id_to_data_map() {
            return std::get<S - 1>(_id_to_data_map);
        }

    private:
        core::tuple<
            core::hash_map<path_t, path_val_t<1>>,
            core::hash_map<path_t, path_val_t<2>>,
            core::hash_map<path_t, path_val_t<3>>,
            core::hash_map<path_t, path_val_t<4>>>      _path_to_id_map;

        core::tuple<
            core::hash_map<raw_id_t, texture_val_t<1>>,
            core::hash_map<raw_id_t, texture_val_t<2>>,
            core::hash_map<raw_id_t, texture_val_t<3>>,
            core::hash_map<raw_id_t, texture_val_t<4>>> _id_to_data_map;

        path_t _textures_dir;

    public:
        DECLARE_GET(textures_dir)
    };


    /**
     * @brief Represents a texture in top level grx_texture_mgr storage
     *
     * All copy operations don't make a real copy of the texture
     *
     * @tparam S - color channels count
     */
    template <size_t S>
    class grx_texture_id {
    public:
        grx_texture_id(grx_texture_id&& t) noexcept: _raw_id(t._raw_id), _parent_mgr(std::move(t._parent_mgr)) {
            t._raw_id = grx_texture<S>::no_name;
        }

        ~grx_texture_id() {
            if (auto mgr = _parent_mgr.lock()) {
                mgr->decrement_usages<S>(_raw_id);
            } else if (_raw_id != grx_texture<S>::no_name) {
                //LOG_WARNING("~grx_texture_id(): Texture with id {} active but associated grx_texture_mgr was deleted", _raw_id);
            }
        }

        grx_texture_id(const grx_texture_id& v): _raw_id(v._raw_id), _parent_mgr(v._parent_mg) {
            if (auto mgr = _parent_mgr.lock()) {
                mgr->increment_usages<S>(_raw_id);
            } else {
                LOG_WARNING("grx_texture_id(): Texture with id {} active but associated grx_texture_mgr was deleted", _raw_id);
            }
        }

        grx_texture_id& operator= (const grx_texture_id& v) {
            if (auto mgr = _parent_mgr.lock())
                mgr->decrement_usages<S>(_raw_id);

            _raw_id     = v._raw_id;
            _parent_mgr = v._parent_mgr;

            if (auto mgr = _parent_mgr.lock()) {
                mgr->increment_usages<S>(_raw_id);
            } else {
                LOG_WARNING("grx_texture_id& operator=(): Texture with id {} active but associated grx_texture_mgr was deleted", _raw_id);
            }

            return *this;
        }

        grx_texture_id& operator= (grx_texture_id&& v) noexcept {
            if (auto mgr = _parent_mgr.lock())
                mgr->decrement_usages<S>(_raw_id);

            _raw_id     = v._raw_id;
            _parent_mgr = std::move(v._parent_mgr);

            v._raw_id = grx_texture<S>::no_name;

            return *this;
        }

        /**
         * @brief Bind texture. Equivalent of glBindTexture
         */
        void bind() const {
            grx_texture_helper::bind_texture(_raw_id);
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
            grx_texture_helper::bind_unit(_raw_id, number);
        }

        /**
         * @brief Sets load significance for associated texture
         *
         * @param load_significance - load significance. See grx::texture_load_significance
         */
        void set_load_significance(texture_load_significance load_significance) {
            if (auto mgr = _parent_mgr.lock())
                mgr->id_to_data_map<S>()[_raw_id].load_significance = load_significance;
            else
                LOG_ERROR("set_load_significance(): Texture with id {} active but associated grx_texture_mgr was deleted");
        }

        /**
         * @brief Gets usages count of associated texture
         *
         * @return usages count
         */
        [[nodiscard]]
        uint get_usages() const {
            if (auto mgr = _parent_mgr.lock())
                return mgr->id_to_data_map<S>()[_raw_id].usages;
            else {
                LOG_ERROR("get_usages(): Texture with id {} active but associated grx_texture_mgr was deleted");
                return 0;
            }
        }

    protected:
        friend grx_texture_mgr;

        template <size_t, bool>
        friend class grx_texture_id_future;

        grx_texture_id(uint raw_id, core::weak_ptr<class grx_texture_mgr>&& parent):
            _raw_id(raw_id), _parent_mgr(core::move(parent)) {}

    private:
        uint                                  _raw_id;
        core::weak_ptr<class grx_texture_mgr> _parent_mgr;
    };


    namespace texture_future_id_details {
        template <size_t S, bool Opt>
        struct image_s;

        template <size_t S>
        struct image_s<S, true> {
            using color_map  = core::try_opt<grx_color_map<uint8_t, S>>;
            using texture    = core::try_opt<grx_texture<S>>;
            using texture_id = core::try_opt<grx_texture_id<S>>;
        };
        template <size_t S>
        struct image_s<S, false> {
            using color_map  = grx_color_map<uint8_t, S>;
            using texture    = grx_texture<S>;
            using texture_id = grx_texture_id<S>;
        };
    }


    template <size_t S, bool Optional = true>
    class grx_texture_id_future {
    public:
        using color_map_t  = typename texture_future_id_details::image_s<S, Optional>::color_map;
        using texture_t    = typename texture_future_id_details::image_s<S, Optional>::texture;
        using texture_id_t = typename texture_future_id_details::image_s<S, Optional>::texture_id;
        using future_t     = core::job_future<color_map_t>;

        grx_texture_id_future(const grx_texture_id_future&) = delete;
        grx_texture_id_future& operator=(const grx_texture_id_future&) = delete;

        grx_texture_id_future(grx_texture_id_future&&) noexcept = default;
        grx_texture_id_future& operator=(grx_texture_id_future&& f) noexcept = default;

        grx_texture_id_future(texture_id_t&& v) noexcept: _storage(std::move(v)) {}

        ~grx_texture_id_future() = default;

        grx_texture_id_future(future_t&& v, core::weak_ptr<grx_texture_mgr>&& mgr, core::string&& path) noexcept:
            _storage(std::move(v)), _parent(std::move(mgr)), _path(std::move(path)) {}

        [[nodiscard]]
        bool is_ready() const {
            if (_storage.index() == 0)
                return std::get<future_t>(_storage) / core::is_ready();
            else if (_storage.index() == 1)
                return true;
            else
                return false;
        }

    private:
        template <typename T>
        static auto& unwrap_if_optional(T& v) {
            if constexpr (core::is_specialization<T, core::try_opt>::value)
                return v.value();
            else
                return v;
        }

        template <bool Opt>
        texture_id_t _get() {
            color_map_t map = std::get<future_t>(_storage).get();

            if (auto mgr = _parent.lock()) {
                if constexpr (Opt)
                    if (!map)
                        return {map.exception_ptr()};

                auto [path_pos, was_inserted] = mgr->path_to_id_map<S>().emplace(_path, grx_texture_mgr::path_val_t<S>());
                auto& path_data = path_pos->second;

                if (!was_inserted) {
                    auto  id   = path_data.id;
                    auto& data = mgr->id_to_data_map<S>()[id];

                    if (data.texture) {
                        data.usages++;
                        return grx_texture_id<S>(id, core::weak_ptr(mgr));
                    } else if (path_data.cached_color_map) {
                        path_data.cached_color_map.reset();
                    }
                }

                auto texture = grx_texture<S>(unwrap_if_optional(map));
                path_data.id = texture.raw_id();
                auto& data   = mgr->id_to_data_map<S>()[texture.raw_id()];
                data.path    = _path;
                data.texture = std::move(texture);
                data.usages++;

                return grx_texture_id<S>(path_data.id, core::weak_ptr(mgr));
            }
            else {
                if constexpr (Opt) {
                    return {std::runtime_error("grx_texture_mgr was deleted and grx_texture_id is invalid.")};
                } else {
                    throw std::runtime_error("grx_texture_mgr was deleted and grx_texture_id is invalid.");
                }
            }
        }

    public:

        texture_id_t get() {
            if (_storage.index() == 0) {
                return _get<Optional>();
            }
            else if (_storage.index() == 1) {
                auto tex_id = std::move(std::get<texture_id_t>(_storage));
                _storage = false;
                return tex_id;
            }
            else
                throw std::future_error(std::future_errc::future_already_retrieved);
        }

        [[nodiscard]]
        bool valid() const {
            if (_storage.index() == 0)
                return std::get<future_t>(_storage).valid();
            else if (_storage.index() == 1)
                return true;
            else
                return false;
        }

        template <typename Rep, typename Period>
        core::job_future_status wait_for(const core::duration<Rep, Period>& duration) const {
            if (_storage.index() == 0)
                return std::get<future_t>(_storage).wait_for(duration);
            else if (_storage.index() == 1)
                return core::job_future_status::ready;
            else
                throw std::future_error(std::future_errc::future_already_retrieved);
        }

        template <typename Clock, typename Duration>
        core::job_future_status wait_until(const core::time_point<Clock, Duration>& time_point) const {
            if (_storage.index() == 0)
                return std::get<future_t>(_storage).wait_until(time_point);
            else if (_storage.index() == 1)
                return core::job_future_status::ready;
            else
                throw std::future_error(std::future_errc::future_already_retrieved);
        }

    private:
        core::variant<future_t, texture_id_t, bool> _storage;
        core::weak_ptr<grx_texture_mgr>             _parent;
        core::string                                _path;
    };


    /*
    namespace grx_texture_type {
        enum grx_texture_type : size_t {
            diffuse = 0,
            normal,
            specular,
            types_count
        };
    }

    class grx_texture_set {
    public:
        grx_texture_set(size_t count = 1): _textures(count, core::pair{grx_texture(), static_cast<uniform_id_t>(-1)}) {
            PeRelRequireF(count < 32, "Wrong textures count {} (Must be < 32)", count);
        }

        grx_texture& get_or_create(size_t type_or_position) {
            PeRelRequireF(type_or_position < 32, "Wrong texture position {} (Must be < 32)", type_or_position);

            if (_textures.size() <= type_or_position) {
                _textures.resize(type_or_position + 1, core::pair{grx_texture(), static_cast<uniform_id_t>(-1)});
                // Reload uniforms
                _cached_program = static_cast<shader_program_id_t>(-1);
            }

            return _textures[type_or_position].first;
        }

        grx_texture& diffuse() {
            return get_or_create(grx_texture_type::diffuse);
        }

        grx_texture& normal() {
            return get_or_create(grx_texture_type::normal);
        }

        grx_texture& specular() {
            return get_or_create(grx_texture_type::specular);
        }

        [[nodiscard]]
        grx_texture& texture(size_t type_or_position) {
            return _textures.at(type_or_position).first;
        }

        [[nodiscard]]
        const grx_texture& texture(size_t type_or_position) const {
            return _textures.at(type_or_position).first;
        }

        [[nodiscard]]
        const grx_texture& diffuse() const {
            return _textures.at(grx_texture_type::diffuse).first;
        }

        [[nodiscard]]
        const grx_texture& normal() const {
            return _textures.at(grx_texture_type::normal).first;
        }

        [[nodiscard]]
        const grx_texture& specular() const {
            return _textures.at(grx_texture_type::specular).first;
        }

        void bind(shader_program_id_t program_id);

    private:
        core::vector<core::pair<grx_texture, uniform_id_t>> _textures;
        shader_program_id_t        _cached_program = static_cast<shader_program_id_t>(-1);
    };


    class grx_texture_mgr {
    public:
        core::optional<grx_texture> load(core::string_view path);
        core::optional<grx_texture> load(const core::config_manager& config_mgr, core::string_view path);

        grx_texture load_unwrap(core::string_view path);
        grx_texture load_unwrap(const core::config_manager& config_mgr, core::string_view path);

    private:
        std::mutex _texture_ids_mutex;
        core::hash_map<core::string, grx_texture> _texture_ids;
    };
    */

} // namespace grx

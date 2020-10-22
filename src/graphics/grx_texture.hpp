#pragma once

#include <core/types.hpp>
#include <core/vec.hpp>
#include <core/assert.hpp>
#include <core/time.hpp>

#include "grx_types.hpp"
#include "grx_color_map.hpp"

namespace grx
{
    enum class texture_access {
        read, write, readwrite
    };

    namespace grx_texture_helper {
        uint create_texture();
        uint create_texture(uint w, uint h, uint channels);
        uint create_texture(uint w, uint h, uint channels, bool is_float, const void* color_map_data);

        void delete_texture(uint name);
        void generate_storage(uint name, uint w, uint h, uint channels);
        void set_storage(uint name, uint w, uint h, uint channels, bool is_float, const void* color_map_data);

        void copy_texture(uint dst_name, uint src_name, uint w, uint h);

        void get_texture(void* dst, uint src_name, uint x, uint y, uint channels, bool is_float);

        void bind_unit         (uint name, uint number);
        void active_texture    (uint number);
        void bind_texture      (uint name);
        void bind_image_texture(uint unit, uint name, int level, texture_access access, uint channels);
    }

    /**
     * @brief Represents a texture in the video memory
     *
     * @tparam S - count of pixel components (1 - red, 2 - rg, 3 - rgb, 4 - rgba)
     */
    template <size_t S>
    class grx_texture {
    public:
        static_assert(S > 0 && S <= 4, "S must be 0 < S <= 4");

        static constexpr const uint no_name = std::numeric_limits<uint>::max();


        /**
         * @brief Gets channels count
         *
         * @return channels count
         */
        static constexpr size_t channels_count() {
            return S;
        }

        /**
         * @brief Construct empty texture
         *
         * @param size - size of the texture
         */
        grx_texture(const core::vec2u& size): _size(size) {
            _gl_name = grx_texture_helper::create_texture(
                    _size.x(), _size.y(), static_cast<uint>(S));
        }

        /**
         * @brief Constuct a texture from color map with same color format and size
         *
         * @tparam T - type of pixel component. Must be float or uint8_t
         * @param color_map - color map for constructing texture
         */
        template <typename T>
        grx_texture(const grx_color_map<T, S>& color_map): _size(color_map.size()) {
            static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, float>, "T must be uint8_t or float only");

            constexpr size_t pixel_size = S * sizeof(T);

            if ((color_map.size().x() * pixel_size) % 4 == 0) {
                _gl_name = grx_texture_helper::create_texture(
                        _size.x(), _size.y(), static_cast<uint>(S), core::FloatingPoint<T>, color_map.data());
            }
            else {
                LOG_WARNING("PERFORMANCE: color_map with size {} violate GL_UNPACK_ALIGNMENT and will be resized.", color_map.size());
                uint new_width = color_map.size().x();

                if constexpr (pixel_size == 1)
                    new_width += 4 - (new_width % 4);
                else if constexpr (pixel_size == 2)
                    ++new_width;
                else if constexpr (pixel_size == 3)
                    new_width += 4 - (new_width % 4);
                else if constexpr (pixel_size == 6) // NOLINT
                    ++new_width;
                else
                    PeRelAbort();

                _size = vec2u{new_width, _size.y()};
                auto new_map = color_map.get_resized(_size);

                _gl_name = grx_texture_helper::create_texture(
                        _size.x(), _size.y(), static_cast<uint>(S), core::FloatingPoint<T>, new_map.data());
            }
        }

        template <typename T>
        grx_texture& operator= (const grx_color_map<T, S>& color_map) {
            static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, float>, "T must be uint8_t or float only");
            _size = color_map.size();

            constexpr size_t pixel_size = S * sizeof(T);

            if ((color_map.size().x() * pixel_size) % 4 == 0) {
                if (_size != color_map.size() || _gl_name == no_name) {
                    grx_texture_helper::delete_texture(_gl_name);
                    _gl_name = grx_texture_helper::create_texture(
                            _size.x(), _size.y(), static_cast<uint>(S), core::FloatingPoint<T>, color_map.data());
                }
                else {
                    grx_texture_helper::set_storage(
                            _gl_name, _size.x(), _size.y(), static_cast<uint>(S), core::FloatingPoint<T>, color_map.data());
                }
            }
            else {
                LOG_WARNING("PERFORMANCE: color_map with size {} {} violate GL_UNPACK_ALIGNMENT and will be resized.");
                uint new_width = color_map.size().x();

                if constexpr (pixel_size == 1)
                    new_width += 4 - (new_width % 4);
                else if constexpr (pixel_size == 2)
                    ++new_width;
                else if constexpr (pixel_size == 3)
                    new_width += 4 - (new_width % 4);
                else if constexpr (pixel_size == 6)
                    ++new_width;
                else
                    PeRelAbort();

                _size = vec2u{new_width, _size.y()};
                auto new_map = color_map.get_resized(_size);

                if (_size != new_map.size() || _gl_name == no_name) {
                    grx_texture_helper::delete_texture(_gl_name);
                    _gl_name = grx_texture_helper::create_texture(
                            _size.x(), _size.y(), static_cast<uint>(S), core::FloatingPoint<T>, new_map.data());
                }
                else {
                    grx_texture_helper::set_storage(
                            _gl_name, _size.x(), _size.y(), static_cast<uint>(S), core::FloatingPoint<T>, new_map.data());
                }
            }

            return *this;
        }

        /**
         * @brief Copy-constructor
         *
         * @param texture - texture to be copied
         */
        grx_texture(const grx_texture& texture): _size(texture._size) {
            _gl_name = grx_texture_helper::create_texture(
                    _size.x(), _size.y(), static_cast<uint>(S));

            grx_texture_helper::copy_texture(
                    _gl_name, texture._gl_id, _size.x(), _size.y());
        }

        /**
         * @brief Copy assignment
         *
         * Assignment with same size textures is optimized
         *
         * @param texture - texture to be copied
         */
        grx_texture& operator= (const grx_texture& texture) {
            _size = texture._size;

            if (_size != texture._size || _gl_name == no_name) {
                grx_texture_helper::delete_texture(_gl_name);

                _gl_name = grx_texture_helper::create_texture(
                        _size.x(), _size.y(), static_cast<uint>(S));

                grx_texture_helper::copy_texture(
                        _gl_name, texture._gl_id, _size.x(), _size.y());
            }

            grx_texture_helper::copy_texture(
                    _gl_name, texture._gl_id, _size.x(), _size.y());

            return *this;
        }

        /**
         * @brief Move constuctor
         *
         * Warning! Result of any operation with grx_texture after std::move(grx_texture) are undefined
         *
         * @param texture - texture to be moved
         */
        grx_texture(grx_texture&& texture) noexcept : _gl_name(texture._gl_name), _size(texture._size) {
            texture._gl_name = no_name;
        }

        /**
         * @brief Move assignment
         *
         * Warning! Result of any operation with grx_texture after std::move(grx_texture) are undefined
         *
         * @param texture - texture to be copied
         */
        grx_texture& operator= (grx_texture&& texture) noexcept {
            if (_gl_name == no_name)
                grx_texture_helper::delete_texture(_gl_name);

            _gl_name = texture._gl_name;
            _size    = texture._size;

            texture._gl_name = no_name;

            return *this;
        }

        /*
         * @brief Destructor
         */
        ~grx_texture() {
            grx_texture_helper::delete_texture(_gl_name);
        }

        /**
         * @brief Activate texture
         *
         * Equivalent of glActiveTexture
         *
         * @tparam N - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         */
        template <uint N>
        void activate() {
            static_assert(N < 32, "N must be < 32");
            grx_texture_helper::active_texture(N);
        }

        /**
         * @brief Activate texture
         *
         * Equivalent of glActiveTexture
         *
         * @param number - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         */
        void activate(uint number) {
            Expects(number < 32);
            grx_texture_helper::active_texture(number);
        }

        /**
         * @brief Bind texture. Equivalent of glBindTexture
         */
        void bind() {
            grx_texture_helper::bind_texture(_gl_name);
        }

        /**
         * @brief Activate texture and bind
         *
         * Equivalent of glActiveTexture and glBindTexture calls (or glBindTextureUnit)
         *
         * @tparam N - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         */
        template <uint N>
        void bind_unit() {
            static_assert(N < 32, "N must be < 32"); // NOLINT
            grx_texture_helper::bind_unit(_gl_name, N);
        }

        /**
         * @brief Activate texture and bind
         *
         * Equivalent of glActiveTexture and glBindTexture calls (or glBindTextureUnit)
         *
         * @param number - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         */
        void bind_unit(uint number) {
            Expects(number < 32);
            grx_texture_helper::bind_unit(_gl_name, number);
        }

        /**
         * @brief Bind a level of the texture
         *
         * Equivalent of glBindImageTexture call
         *
         * @param N      - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         * @param level  - the level of the texture
         * @param access - access specifier
         */
        template <uint N>
        void bind_level(int level, texture_access access = texture_access::read) {
            static_assert(N < 32, "N must be < 32"); // NOLINT
            grx_texture_helper::bind_image_texture(N, _gl_name, level, access, static_cast<uint>(S));
        }

        /**
         * @brief Bind a level of the texture
         *
         * Equivalent of glBindImageTexture call
         *
         * @param number - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         * @param level  - the level of the texture
         * @param access - access specifier
         */
        void bind_level(uint number, int level, texture_access access = texture_access::read) {
            Expects(number < 32);
            grx_texture_helper::bind_image_texture(number, _gl_name, level, access, static_cast<uint>(S));
        }

        /**
         * @brief Load texture to color map from video memory
         *
         * @tparam - type of pixel component
         * @return color map with texture data
         */
        template <typename T = uint8_t>
        [[nodiscard]]
        grx_color_map<T, S> to_color_map() const {
            grx_color_map<T, S> result(_size);

            grx_texture_helper::get_texture(
                    result.data(), _gl_name, _size.x(), _size.y(), static_cast<uint>(S), core::FloatingPoint<T>);

            return result;
        }

        /**
         * @brief Gets OpenGl texture id
         *
         * @return raw OpenGl texture id
         */
        [[nodiscard]]
        uint raw_id() const {
            return _gl_name;
        }

        /**
         * @brief Gets size of the texture
         *
         * @return size of the texture
         */
        [[nodiscard]]
        const core::vec2u& size() const {
            return _size;
        }

    private:
        uint        _gl_name = no_name;
        core::vec2u _size;
    };


    /**
     * @brief Represents a texture in the video memory
     *
     * This is a type-erasure class, and it can store textures of any color types
     *
     */
    class grx_texture_generic {
    private:
        struct holder_base {
            virtual ~holder_base() = default;
            [[nodiscard]]
            virtual size_t channels_count() const   = 0;
            virtual void   activate(uint) const     = 0;
            virtual void   bind() const             = 0;
            virtual void   bind_unit(uint) const    = 0;
            [[nodiscard]]
            virtual uint   raw_id() const           = 0;
            [[nodiscard]]
            virtual const core::vec2u& size() const = 0;
            [[nodiscard]]
            virtual core::unique_ptr<holder_base> get_copy() const = 0;
        };

        template <typename T>
        struct holder : holder_base {
            explicit holder(T&& h) noexcept: val(std::forward<T>(h)) {}
            holder(const T& h): val(h) {}

            [[nodiscard]]
            size_t channels_count() const override {
                return T::channels_count();
            }

            void activate(uint num) const override {
                val.activate(num);
            }

            void bind() const override {
                val.bind();
            }

            void bind_unit(uint num) const override {
                val.bind_unit(num);
            }

            [[nodiscard]]
            uint raw_id() const override {
                return val.raw_id();
            }

            [[nodiscard]]
            const core::vec2u& size() const override {
                return val.size();
            }

            [[nodiscard]]
            core::unique_ptr<holder_base> get_copy() const override {
                return core::make_unique<holder<T>>(val);
            }

            T val;
        };

    public:
        template <size_t S>
        grx_texture_generic(grx_texture<S>&& texture):
            _holder(core::make_unique<holder<grx_texture<S>>>(std::move(texture))) {}

        template <size_t S>
        grx_texture_generic& operator= (grx_texture<S>&& texture) noexcept {
            _holder = core::make_unique<holder<grx_texture<S>>>(std::move(texture));
            return *this;
        }

        template <size_t S>
        grx_texture_generic(const grx_texture<S>& texture):
            _holder(core::make_unique<holder<grx_texture<S>>>(texture)) {}

        template <size_t S>
        grx_texture_generic& operator= (const grx_texture<S>& texture) {
            _holder = core::make_unique<holder<grx_texture<S>>>(texture);
            return *this;
        }

        grx_texture_generic(grx_texture_generic&& v) noexcept: _holder(std::move(v._holder)) {}

        grx_texture_generic& operator= (grx_texture_generic&& v) noexcept {
            _holder = std::move(v._holder);
            return *this;
        }

        grx_texture_generic(const grx_texture_generic& texture): _holder(texture._holder->get_copy()) {}

        grx_texture_generic& operator= (const grx_texture_generic& texture) {
            _holder = texture._holder->get_copy();
            return *this;
        }

        /**
         * @brief Gets channels count
         *
         * @return channels count
         */
        [[nodiscard]]
        size_t channels_count() const {
            return _holder->channels_count();
        }

        /**
         * @brief Activate texture
         *
         * Equivalent of glActiveTexture
         *
         * @param number - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         */
        void activate(uint number) {
            _holder->activate(number);
        }

        /**
         * @brief Bind texture. Equivalent of glBindTexture
         */
        void bind() {
            _holder->bind();
        }

        /**
         * @brief Activate texture and bind
         *
         * Equivalent of glActiveTexture and glBindTexture calls (or glBindTextureUnit)
         *
         * @param number - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         */
        void bind_unit(uint number) {
            _holder->bind_unit(number);
        }

        /**
         * @brief Gets OpenGl texture id
         *
         * @return raw OpenGl texture id
         */
        [[nodiscard]]
        uint raw_id() const {
            return _holder->raw_id();
        }

        /**
         * @brief Gets size of the texture
         *
         * @return size of the texture
         */
        [[nodiscard]]
        const core::vec2u& size() const {
            return _holder->size();
        }

        /**
         * @brief Try cast to T pointer
         *
         * @tparam T - target type
         *
         * @return pointer to T if cast successful or nullptr
         */
        template <typename T>
        [[nodiscard]]
        T* ptr_cast() {
            auto ptr = dynamic_cast<holder<T>*>(_holder.get());
            return ptr ? &ptr->val : nullptr;
        }

        /**
         * @brief Try cast to const T pointer
         *
         * @tparam T - target type
         *
         * @return const pointer to T if cast successful or nullptr
         */
        template <typename T>
        [[nodiscard]]
        const T* ptr_cast() const {
            auto ptr = dynamic_cast<holder<T>*>(_holder.get());
            return ptr ? &ptr->val : nullptr;
        }

        /**
         * @brief Try cast to T
         *
         * throws std::runtime_error if cast impossible
         *
         * @tparam T - target type
         *
         * @return reference to T if cast successful
         */
        template <typename T>
        T& cast() {
            auto res = ptr_cast<T>();
            if (!res)
                throw std::runtime_error("Invalid grx_texture_generic cast");
            return *res;
        }

        /**
         * @brief Try cast to T
         *
         * Throws std::runtime_error if cast impossible
         *
         * @tparam T - target type
         *
         * @return const reference to T if cast successful
         */
        template <typename T>
        const T& cast() const {
            auto res = ptr_cast<T>();
            if (!res)
                throw std::runtime_error("Invalid grx_texture_generic cast");
            return *res;
        }

    private:
        core::unique_ptr<holder_base> _holder;
    };


    template <size_t S, typename T>
    class grx_texture_future {
    public:
        using future_t = core::job_future<T>;

        grx_texture_future(future_t&& init) noexcept: _future(std::move(init)) {}

        [[nodiscard]]
        bool is_ready() const {
            return _future / core::is_ready();
        }

        template <typename Enable = void> requires (!core::Optional<T>)
        grx_texture<S> get() {
            return grx_texture<S>(std::move(_future.get()));
        }

        template <typename Enable = void> requires core::Optional<T>
        core::optional<grx_texture<S>> get() {
            auto map = std::move(_future.get());
            if (map)
                return grx_texture<S>(std::move(*map));
            else
                return core::nullopt;
        }

        [[nodiscard]]
        bool valid() const {
            return _future.valid();
        }

        template <typename Rep, typename Period>
        core::job_future_status wait_for(const core::duration<Rep, Period>& duration) const {
            return _future.wait_for(duration);
        }

        template <typename Clock, typename Duration>
        core::job_future_status wait_until(const core::time_point<Clock, Duration>& time_point) const {
            return _future.wait_until(time_point);
        }

    private:
        future_t _future;
    };

    /**
     * @brief Load texture from bytes
     *
     * Throws std::runtime_error if bytes not contain valid image
     *
     * @tparam T - color type
     * @param bytes - bytes to be load
     *
     * @return result texture
     */
    template <core::MathVector T = color_rgb>
    grx_texture<T::size()>
    load_texture_from_bytes(core::span<core::byte> bytes) {
        return grx_texture<T::size()>(load_color_map_from_bytes<T>(bytes));
    }

    /**
     * @brief Load texture from file
     *
     * Throws std::runtime_error if image file not contain valid image
     *
     * @tparam T - color type
     * @param file_path - path to image file
     *
     * @return result texture if loading successful or std::nullopt
     */
    template <core::MathVector T = color_rgb>
    core::optional<grx_texture<T::size()>>
    load_texture(const core::string& file_path) {
        if (auto map = load_color_map<T>(file_path))
            return grx_texture<T::size()>(*map);
        else
            return core::nullopt;
    }

    /**
     * @brief Load texture from file
     *
     * Throws std::runtime_error if image file not contain valid image
     * or file not found
     *
     * @tparam T - color type
     * @param file_path - path to image file
     *
     * @return result texture
     */
    template <core::MathVector T = color_rgb>
    grx_texture<T::size()>
    load_texture_unwrap(const core::string& file_path) {
        return grx_texture<T::size()>(load_color_map_unwrap<T>(file_path));
    }

    /**
     * @brief Load texture from file asynchronously
     *
     * Throws std::runtime_error if image file not contain valid image
     *
     * @tparam T - color type
     * @param file_path - path to image file
     *
     * @return future to texture on success or future with std::nullopt
     */
    template <core::MathVector T = color_rgb>
    grx_texture_future<T::size(), core::optional<grx_color_map<uint8_t, T::size()>>>
    load_texture_async(const core::string& file_path) {
        return grx_texture_future<T::size(), core::optional<grx_color_map<uint8_t, T::size()>>>(
                load_color_map_async<T>(file_path));
    }

    /**
     * @brief Load texture from file asynchronously
     *
     * Throws std::runtime_error if image file not contain valid image
     * or file not found
     *
     * @tparam T - color type
     * @param file_path - path to image file
     *
     * @return future to texture
     */
    template <core::MathVector T = color_rgb>
    grx_texture_future<T::size(), grx_color_map<uint8_t, T::size()>>
    load_texture_async_unwrap(const core::string& file_path) {
        return grx_texture_future<T::size(), grx_color_map<uint8_t, T::size()>>(
                load_color_map_async_unwrap<T>(file_path));
    }

} // namespace grx

#pragma once

#include <core/types.hpp>
#include <core/vec.hpp>
#include <core/assert.hpp>
#include <core/time.hpp>

#include "grx_types.hpp"
#include "grx_color_map.hpp"

namespace grx
{
    namespace grx_texture_helper {
        uint generate_gl_texture();
        void delete_gl_texture(uint id);
        void reset_texture(uint id, uint channels_count, uint w, uint h, bool is_float, const void* color_map_data, bool gen_mipmap);
        void setup_texture(uint id, uint channels_count, uint w, uint h, bool is_float, const void* color_map_data, bool gen_mipmap);
        void gl_active_texture(uint num);
        void gl_bind_texture(uint id);
        void copy_texture(uint dst_id, uint src_id, uint w, uint h);
        void get_texture(void* dst, uint channels_count, bool is_float);
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

        static constexpr const uint no_id = std::numeric_limits<uint>::max();


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
            _gl_id = grx_texture_helper::generate_gl_texture();
            grx_texture_helper::setup_texture(
                    _gl_id,
                    static_cast<uint>(S),
                    _size.x(),
                    _size.y(),
                    false,
                    nullptr,
                    true);
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
            _gl_id = grx_texture_helper::generate_gl_texture();
            grx_texture_helper::setup_texture(
                    _gl_id,
                    static_cast<uint>(S),
                    _size.x(),
                    _size.y(),
                    core::FloatingPoint<T>,
                    color_map.data(),
                    true);
        }

        template <typename T>
        grx_texture& operator= (const grx_color_map<T, S>& color_map) {
            static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, float>, "T must be uint8_t or float only");
            _size = color_map.size();

            if (_size != color_map.size() || _gl_id == no_id) {
                grx_texture_helper::delete_gl_texture(_gl_id);
                grx_texture_helper::setup_texture(
                        _gl_id,
                        static_cast<uint>(S),
                        _size.x(),
                        _size.y(),
                        core::FloatingPoint<T>,
                        color_map.data(),
                        true);
            }
            else {
                reset_texture(
                        _gl_id,
                        static_cast<uint>(S),
                        _size.x(),
                        _size.y(),
                        core::FloatingPoint<T>,
                        color_map.data(),
                        true);
            }
        }

        /**
         * @brief Copy-constructor
         *
         * @param texture - texture to be copied
         */
        grx_texture(const grx_texture& texture): _size(texture._size) {
            _gl_id = grx_texture_helper::generate_gl_texture();

            grx_texture_helper::setup_texture(
                    _gl_id,
                    static_cast<uint>(S),
                    _size.x(),
                    _size.y(),
                    false,
                    nullptr,
                    true);

            grx_texture_helper::copy_texture(
                    _gl_id,
                    texture._gl_id,
                    _size.x(),
                    _size.y());
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

            if (_size != texture._size || _gl_id == no_id) {
                grx_texture_helper::delete_gl_texture(_gl_id);
                grx_texture_helper::setup_texture(
                        _gl_id,
                        static_cast<uint>(S),
                        _size.x(),
                        _size.y(),
                        false,
                        nullptr,
                        true);
            }

            grx_texture_helper::copy_texture(
                    _gl_id,
                    texture._gl_id,
                    _size.x(),
                    _size.y());

            return *this;
        }

        /**
         * @brief Move constuctor
         *
         * Warning! Result of any operation with grx_texture after std::move(grx_texture) are undefined
         *
         * @param texture - texture to be moved
         */
        grx_texture(grx_texture&& texture) noexcept : _gl_id(texture._gl_id), _size(texture._size) {
            texture._gl_id = no_id;
        }

        /**
         * @brief Move assignment
         *
         * Warning! Result of any operation with grx_texture after std::move(grx_texture) are undefined
         *
         * @param texture - texture to be copied
         */
        grx_texture& operator= (grx_texture&& texture) noexcept {
            _gl_id = texture._gl_id;
            _size  = texture._size;

            texture._gl_id = no_id;

            return *this;
        }

        /*
         * @brief Destructor
         */
        ~grx_texture() {
            grx_texture_helper::delete_gl_texture(_gl_id);
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
            grx_texture_helper::gl_active_texture(N);
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
            grx_texture_helper::gl_active_texture(number);
        }

        /**
         * @brief Bind texture. Equivalent of glBindTexture
         */
        void bind() {
            grx_texture_helper::gl_bind_texture(_gl_id);
        }

        /**
         * @brief Activate texture and bind
         *
         * Equivalent of glActiveTexture and glBindTexture calls
         *
         * @tparam N - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         */
        template <uint N>
        void activate_and_bind() {
            activate<N>();
            bind();
        }

        /**
         * @brief Activate texture and bind
         *
         * Equivalent of glActiveTexture and glBindTexture calls
         *
         * @param number - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         */
        void activate_and_bind(uint number) {
            activate(number);
            bind();
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
            grx_texture_helper::gl_bind_texture(_gl_id);
            grx_texture_helper::get_texture(result.data(), S, core::FloatingPoint<T>);
            return result;
        }

        /**
         * @brief Gets OpenGl texture id
         *
         * @return raw OpenGl texture id
         */
        [[nodiscard]]
        uint raw_id() const {
            return _gl_id;
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
        uint        _gl_id = no_id;
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
            virtual size_t channels_count() const   = 0;
            virtual void   activate(uint)           = 0;
            virtual void   bind()                   = 0;
            virtual void   activate_and_bind(uint)  = 0;
            virtual uint   raw_id() const           = 0;
            virtual const core::vec2u& size() const = 0;
            virtual core::unique_ptr<holder_base> get_copy() const = 0;
        };

        template <typename T>
        struct holder : holder_base {
            explicit holder(T&& h) noexcept: val(std::forward<T>(h)) {}
            holder(const T& h): val(h) {}

            size_t channels_count() const override {
                return T::channels_count();
            }

            void activate(uint num) override {
                val.activate(num);
            }

            void bind() override {
                val.bind();
            }

            void activate_and_bind(uint num) override {
                val.activate_and_bind(num);
            }

            uint raw_id() const override {
                return val.raw_id();
            }

            const core::vec2u& size() const override {
                return val.size();
            }

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
         * Equivalent of glActiveTexture and glBindTexture calls
         *
         * @param number - number of texture (0 - GL_TEXTURE0, 1 - GL_TEXTURE1, etc.)
         */
        void activate_and_bind(uint number) {
            _holder->activate_and_bind(number);
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
        using future_t = std::future<T>;

        grx_texture_future(future_t&& init) noexcept: _future(std::move(init)) {}

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

        bool valid() const {
            return _future.valid();
        }

        template <typename Rep, typename Period>
        std::future_status wait_for(const core::duration<Rep, Period>& duration) const {
            return _future.wait_for(duration);
        }

        template <typename Clock, typename Duration>
        std::future_status wait_until(const core::time_point<Clock, Duration>& time_point) const {
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

#pragma once

#include <ostream>

extern "C" {
#include <stb/stb_image.h>
#include <stb/stb_image_resize.h>
#include <stb/stb_image_write.h>
}

#include "grx_types.hpp"
#include <core/async.hpp>
#include <core/fiber_pool.hpp>
#include <core/container_extensions.hpp>
#include <core/math.hpp>
#include <core/types.hpp>
#include <core/vec.hpp>

namespace grx
{
using core::FloatingPoint;
using core::vec;
using core::vec2u;
using core::zip_view;
using core::u8;
using core::make_unique;

static constexpr u8 COLOR_VAL_MAX = 255;

/**
 * @brief Provides access to the pixel data
 *
 * @tparam T - the type of the color channel value
 * @tparam NPP - channels count
 * @tparam Const - const flag
 */
template <typename T, size_t NPP, bool Const = false>
class grx_color_pixel_view {
public:
    grx_color_pixel_view(const grx_color_pixel_view& v) noexcept = default;
    grx_color_pixel_view& operator=(const grx_color_pixel_view& v) noexcept = default;

    grx_color_pixel_view(grx_color_pixel_view&& v) noexcept = default;
    grx_color_pixel_view& operator=(grx_color_pixel_view&& v) noexcept = default;

    ~grx_color_pixel_view() noexcept = default;

    /**
     * @brief Sets pixel data from the color
     *
     * @tparam TT - the type of the color channel value
     * @param color - the color
     *
     * @return *this
     */
    template <typename TT> requires(!Const)
    grx_color_pixel_view& operator=(const vec<TT, NPP>& color) noexcept {
        vec<T, NPP> result_color;

        if constexpr ((FloatingPoint<TT> && FloatingPoint<T>) || (!FloatingPoint<TT> && !FloatingPoint<T>))
            result_color = static_cast<vec<T, NPP>>(color);
        else if constexpr (!FloatingPoint<TT> && FloatingPoint<T>)
            result_color = static_cast<vec<T, NPP>>(color) / T(COLOR_VAL_MAX);
        else if constexpr (FloatingPoint<TT> && !FloatingPoint<T>)
            result_color = static_cast<vec<T, NPP>>(core::round(color * TT(COLOR_VAL_MAX)));

        memcpy(_data, result_color.v.data(), NPP * sizeof(T));
        return *this;
    }

    /**
     * @brief Gets color from the pixel
     *
     * @return the color
     */
    vec<T, NPP> get() const noexcept {
        vec<T, NPP> color;
        memcpy(color.v.data(), _data, NPP * sizeof(T));
        return color;
    }

    void print(std::ostream& os) const {
        core::string      str = "#";
        vec<uint8_t, NPP> color;

        if constexpr (!FloatingPoint<T>)
            color = static_cast<vec<uint8_t, NPP>>(get());
        else
            color = static_cast<vec<uint8_t, NPP>>(core::round(static_cast<vec<T, NPP>>(get()) * T(COLOR_VAL_MAX)));

        for (auto c : color.v) {
            str.push_back("0123456789abcdef"[c >> 4]);
            str.push_back("0123456789abcdef"[c & 0x0f]); // NOLINT
        }

        os << str;
    }

private:
    template <typename, size_t, bool>
    friend class grx_color_row_view;

    template <typename, size_t, bool>
    friend class grx_color_map_iterator;

    grx_color_pixel_view(T* data) noexcept: _data(data) {}

private:
    T* _data;
};


template <typename T, size_t NPP, bool Const = false>
class grx_color_map_iterator {
public:
    using iterator_category = std::random_access_iterator_tag;
    using difference_type   = std::ptrdiff_t;

    grx_color_map_iterator() noexcept = default;
    grx_color_map_iterator(const grx_color_map_iterator<T, NPP, true>& it) noexcept: _ptr(it._ptr) {}

    grx_color_map_iterator& operator++() noexcept {
        _ptr += NPP;
        return *this;
    }

    grx_color_map_iterator& operator--() noexcept {
        _ptr -= NPP;
        return *this;
    }

    grx_color_map_iterator operator++(int) noexcept {
        auto res = *this;
        _ptr += NPP;
        return res;
    }

    grx_color_map_iterator operator--(int) noexcept {
        auto res = *this;
        _ptr -= NPP;
        return res;
    }

    grx_color_map_iterator& operator+=(difference_type n) noexcept {
        _ptr += n * NPP;
        return *this;
    }

    grx_color_map_iterator& operator-=(difference_type n) noexcept {
        _ptr -= n * NPP;
        return *this;
    }

    grx_color_map_iterator operator+(difference_type n) const noexcept {
        auto res = *this;
        res += n;
        return res;
    }

    grx_color_map_iterator operator-(difference_type n) const noexcept {
        auto res = *this;
        res -= n;
        return res;
    }

    difference_type operator-(const grx_color_map_iterator& it) const noexcept {
        return (_ptr - it._ptr) / NPP;
    }

    bool operator<(const grx_color_map_iterator& it) const noexcept {
        return _ptr < it._ptr;
    }

    bool operator>(const grx_color_map_iterator& it) const noexcept {
        return _ptr > it._ptr;
    }

    bool operator==(const grx_color_map_iterator& it) const noexcept {
        return _ptr == it._ptr;
    }

    bool operator>=(const grx_color_map_iterator& it) const noexcept {
        return !(*this < it);
    }

    bool operator<=(const grx_color_map_iterator& it) const noexcept {
        return !(*this > it);
    }

    bool operator!=(const grx_color_map_iterator& it) const noexcept {
        return !(*this == it);
    }

    grx_color_pixel_view<T, NPP, Const> operator*() const noexcept {
        return grx_color_pixel_view<T, NPP, Const>(_ptr);
    }

    grx_color_pixel_view<T, NPP, Const> operator[](difference_type n) const noexcept {
        return *(*this + n);
    }

private:
    template <typename, size_t>
    friend class grx_color_map;

    template <typename, size_t, bool>
    friend class grx_color_row_view;

    grx_color_map_iterator(T* ptr) noexcept: _ptr(ptr) {}

private:
    T* _ptr = nullptr;
};

/**
 * @brief Provides access to the row of pixels
 *
 * @tparam T - the type of the color channel value
 * @tparam NPP - channels count
 * @tparam Const - const flag
 */
template <typename T, size_t NPP, bool Const = false>
class grx_color_row_view {
public:
    grx_color_pixel_view<T, NPP, Const> operator[](size_t pos) const noexcept {
        return grx_color_pixel_view<T, NPP, Const>(_data + pos * NPP);
    }

    grx_color_pixel_view<T, NPP, Const> at(size_t pos) const {
        if (pos >= _width)
            throw std::out_of_range("Out of range");

        return grx_color_pixel_view<T, NPP, Const>(_data + pos * NPP);
    }

    grx_color_map_iterator<T, NPP, Const> begin() const noexcept {
        return grx_color_map_iterator<T, NPP, Const>(_data);
    }

    grx_color_map_iterator<T, NPP, Const> end() const noexcept {
        return grx_color_map_iterator<T, NPP, Const>(_data + _width * NPP);
    }

private:
    template <typename, size_t>
    friend class grx_color_map;

    grx_color_row_view(T* data, uint width) noexcept: _data(data), _width(width) {}

private:
    T*   _data;
    uint _width;
};


template <typename T, size_t NPP>
class grx_color_map {
public:
    using c_array = T[]; // NOLINT

    grx_color_map(T* raw_data, const vec2u& size): _size(size) {
        _data = make_unique<c_array>(_size.x() * _size.y() * NPP);
        memcpy(_data.get(), raw_data, _size.x() * _size.y() * NPP * sizeof(T));
    }

    grx_color_map(const vec2u& size): _size(size) {
        _data = make_unique<c_array>(_size.x() * _size.y() * NPP);
    }

    grx_color_map(const grx_color_map& map): grx_color_map(map._data.get(), map._size) {}

    grx_color_map& operator=(const grx_color_map& map) {
        _size = map._size;
        _data = make_unique<c_array>(_size.x() * _size.y() * NPP);
        memcpy(_data.get(), map._data.get(), _size.x() * _size.y() * NPP * sizeof(T));

        return *this;
    }

    grx_color_map(grx_color_map&& map) noexcept: _size(map._size), _data(std::move(map._data)) {
        map._data = nullptr;
    }

    grx_color_map& operator=(grx_color_map&& map) noexcept {
        _size     = map._size;
        _data     = std::move(map._data);
        map._data = nullptr;

        return *this;
    }

    ~grx_color_map() noexcept = default;

    grx_color_map_iterator<T, NPP> begin() noexcept {
        return grx_color_map_iterator<T, NPP>(_data.get());
    }

    grx_color_map_iterator<T, NPP> end() noexcept {
        return grx_color_map_iterator<T, NPP>(_data.get() + _size.x() * _size.y() * NPP);
    }

    grx_color_map_iterator<T, NPP, true> begin() const noexcept {
        return grx_color_map_iterator<T, NPP, true>(_data.get());
    }

    grx_color_map_iterator<T, NPP, true> end() const noexcept {
        return grx_color_map_iterator<T, NPP, true>(_data.get() + _size.x() * _size.y() * NPP);
    }

    grx_color_row_view<T, NPP, true> operator[](size_t row_num) const noexcept {
        return grx_color_row_view<T, NPP, true>(_data.get() + row_num * _size.x() * NPP, _size.x());
    }

    grx_color_row_view<T, NPP> operator[](size_t row_num) noexcept {
        return grx_color_row_view<T, NPP>(_data.get() + row_num * _size.x() * NPP, _size.x());
    }

    template <typename IdxT>
    grx_color_pixel_view<T, NPP, true> operator[](vec<IdxT, 2> pos) const noexcept {
        return (*this)[pos.y()][pos.x()];
    }

    template <typename IdxT>
    grx_color_pixel_view<T, NPP, false> operator[](vec<IdxT, 2> pos) noexcept {
        return (*this)[pos.y()][pos.x()];
    }

    template <typename TT>
    [[nodiscard]]
    explicit operator grx_color_map<TT, NPP>() const {
        grx_color_map<TT, NPP> result(_size);

        for (auto& [dst, src] : zip_view(result, *this))
            dst = src.get();

        return result;
    }

    [[nodiscard]]
    const vec2u& size() const noexcept {
        return _size;
    }

    [[nodiscard]]
    const T* data() const noexcept {
        return _data.get();
    }

    [[nodiscard]]
    T* data() noexcept {
        return _data.get();
    }

    grx_color_map get_resized(const vec2u& new_size) const {
        auto output_pixels = make_unique<c_array>(new_size.x() * new_size.y() * NPP);

        if constexpr (core::FloatingPoint<T>)
            stbir_resize_float(_data.get(),
                               static_cast<int>(_size.x()),
                               static_cast<int>(_size.y()),
                               0,
                               output_pixels.get(),
                               static_cast<int>(new_size.x()),
                               static_cast<int>(new_size.y()),
                               0,
                               static_cast<int>(NPP));
        else
            stbir_resize_uint8(_data.get(),
                               static_cast<int>(_size.x()),
                               static_cast<int>(_size.y()),
                               0,
                               output_pixels.get(),
                               static_cast<int>(new_size.x()),
                               static_cast<int>(new_size.y()),
                               0,
                               static_cast<int>(NPP));

        grx_color_map result;
        result._data = std::move(output_pixels);
        result._size = new_size;

        return result;
    }

private:
    grx_color_map() noexcept = default;

private:
    vec2u                _size = {0, 0};
    std::unique_ptr<T[]> _data = nullptr; // NOLINT
};

using grx_color_map_r    = grx_color_map<uint8_t, 1>;
using grx_color_map_rg   = grx_color_map<uint8_t, 2>;
using grx_color_map_rgb  = grx_color_map<uint8_t, 3>;
using grx_color_map_rgba = grx_color_map<uint8_t, 4>;

using grx_float_color_map_r    = grx_color_map<float, 1>;
using grx_float_color_map_rg   = grx_color_map<float, 2>;
using grx_float_color_map_rgb  = grx_color_map<float, 3>;
using grx_float_color_map_rgba = grx_color_map<float, 4>;

/**
 * @brief Loads color_map from bytes
 *
 * @tparam T - the type of the color
 * @param bytes - bytes data
 *
 * @return the color map or exception in failure_opt
 */
template <core::MathVector T = color_rgb>
core::failure_opt<grx_color_map<typename T::value_type, T::size()>>
load_color_map_from_bytes(core::span<core::byte> bytes) {
    static_assert(T::size() <= 4 && T::size() > 0,
                  "Wrong color type. T must be uint8_t or float, Size must be 1 "
                  "<= Size <= 4");

    int w, h, comp; // NOLINT
    int type;       // NOLINT

    if constexpr (T::size() == 1)
        type = STBI_grey;
    else if constexpr (T::size() == 2)
        type = STBI_grey_alpha;
    else if constexpr (T::size() == 3)
        type = STBI_rgb;
    else if constexpr (T::size() == 4)
        type = STBI_rgb_alpha;

    auto img = stbi_load_from_memory(
        reinterpret_cast<const uint8_t*>(bytes.data()), static_cast<int>(bytes.size()), &w, &h, &comp, type); // NOLINT

    if (!img)
        return {std::runtime_error("Can't load image from memory")};

    auto map = grx_color_map<uint8_t, T::size()>(img, static_cast<vec2u>(vec{w, h}));
    stbi_image_free(img);

    if constexpr (std::is_same_v<typename T::value_type, uint8_t>)
        return map;
    else
        return static_cast<grx_color_map<typename T::value_type, T::size()>>(map);
}

/**
 * @brief Loads a color map from the image file
 *
 * @tparam T - the color type
 * @param file_path - a path to the image file
 *
 * @return the read color map or exception in failure_opt
 */
template <core::MathVector T = color_rgb>
[[nodiscard]]
core::failure_opt<grx_color_map<typename T::value_type, T::size()>>
load_color_map(const core::string& file_path) {
    if (auto file = core::read_binary_file(core::path_eval(file_path)))
        return load_color_map_from_bytes<T>(*file);
    else
        return {std::runtime_error("Can't open file \"" + file_path + "\"")};
}

/**
 * @brief Loads a color map from the image file
 *
 * @throw exception if image file can't be read
 *
 * @tparam T - the color type
 * @param file_path - a path to the image file
 *
 * @return the read color map
 */
template <core::MathVector T = color_rgb>
[[nodiscard]]
grx_color_map<typename T::value_type, T::size()>
load_color_map_unwrap(const core::string& file_path) {
    return load_color_map<T>(file_path).value();
}

/**
 * @brief Loads a color map from the image file asynchronously
 *
 * @tparam T - the color type
 * @param file_path - a path to the image file
 *
 * @return the future to the color_map or exception in the failure_opt
 */
template <core::MathVector T = color_rgb>
[[nodiscard]]
core::job_future<core::failure_opt<grx_color_map<typename T::value_type, T::size()>>>
load_color_map_async(const core::string& file_path) {
    return core::submit_job(load_color_map<T>, file_path);
}

/**
 * @brief Loads a color map from the image file asynchronously
 *
 * @throw the returned future throws an exception if image file can't be read
 *
 * @tparam T - the color type
 * @param file_path - a path to the image file
 *
 * @return the future to the color_map
 */
template <core::MathVector T = color_rgb>
[[nodiscard]]
core::job_future<grx_color_map<typename T::value_type, T::size()>>
load_color_map_async_unwrap(const core::string& file_path) {
    return core::submit_job(load_color_map_unwrap<T>, file_path);
}

namespace color_map_save_bytes_helper
{
    inline void write_callback(void* byte_vector, void* data, int size) {
        auto vec      = static_cast<core::vector<core::byte>*>(byte_vector);
        auto old_size = vec->size();
        vec->resize(old_size + static_cast<size_t>(size));
        memcpy(vec->data() + old_size, data, static_cast<size_t>(size)); // NOLINT
    }
} // namespace color_map_save_bytes_helper

static constexpr int default_jpeg_quality = 95;

/**
 * @brief Saves the color map to image bytes
 *
 * @tparam T - the type of the color component
 * @tparam S - the color channels count
 * @param color_map - the color map to be saved
 * @param extension - the image extension
 *
 * @return byte vector with image or exception in failure_opt
 */
template <typename T, size_t S>
[[nodiscard]]
core::failure_opt<core::vector<core::byte>>
save_color_map_to_bytes(const grx_color_map<T, S>& color_map, core::string_view extension = "png") {
    core::vector<core::byte> result;

    using save_func_t = std::function<int(stbi_write_func*, void*, int, int, int, const void*)>;
    save_func_t save_func;

    if (extension == "png")
        save_func = [&](stbi_write_func* f, void* c, int w, int h, int comp, const void* data) {
            return stbi_write_png_to_func(f, c, w, h, comp, data, static_cast<int>(color_map.size().x() * S));
        };
    else if (extension == "bmp")
        save_func = stbi_write_bmp_to_func;
    else if (extension == "tga")
        save_func = stbi_write_tga_to_func;
    else if (extension == "jpg")
        save_func = [](stbi_write_func* f, void* c, int w, int h, int comp, const void* data) {
            return stbi_write_jpg_to_func(f, c, w, h, comp, data, default_jpeg_quality);
        };
    else
        return {std::runtime_error("Can't save image: unsupported extension '" + core::string(extension) + "'")};

    int rc; // NOLINT
    if constexpr (std::is_same_v<T, uint8_t>)
        rc = save_func(color_map_save_bytes_helper::write_callback,
                       &result,
                       static_cast<int>(color_map.size().x()),
                       static_cast<int>(color_map.size().y()),
                       static_cast<int>(S),
                       color_map.data());
    else {
        auto map = static_cast<grx_color_map<uint8_t, S>>(color_map);
        rc       = save_func(color_map_save_bytes_helper::write_callback,
                       &result,
                       static_cast<int>(map.size().x()),
                       static_cast<int>(map.size().y()),
                       static_cast<int>(S),
                       map.data());
    }

    if (rc == 0)
        return {std::runtime_error("Can't save image to bytes")};
    else
        return result;
}

/**
 * @brief Saves the color map to image bytes
 *
 * @throw exception if color map can't be saved
 *
 * @tparam T - the type of the color component
 * @tparam S - the color channels count
 * @param color_map - the color map to be saved
 * @param extension - the image extension
 *
 * @return byte vector with image
 */
template <typename T, size_t S>
[[nodiscard]]
core::vector<core::byte>
save_color_map_to_bytes_unwrap(const grx_color_map<T, S>& color_map, core::string_view extension = "png") {
    return save_color_map_to_bytes(color_map, extension).value();
}

/**
 * @brief Saves the color map to file
 *
 * @tparam T - the type of the color component
 * @tparam S - the color channels count
 * @param color_map - the color map to be saved
 * @param file_path - a path to new file with extension
 *
 * @return the failure_opt with bool value if save successfull or exception_ptr
 */
template <typename T, size_t S>
[[nodiscard]]
core::failure_opt<bool>
save_color_map(const grx_color_map<T, S>& color_map, const core::string& file_path) {
    constexpr size_t minimal_len = 5; // a.bmp, c.jpg, ...

    if (file_path.size() < minimal_len)
        return {std::runtime_error("Can't save image '" + file_path + "': can't determine file format")};

    auto extension = file_path.substr(file_path.size() - 4) / core::transform<core::string>(::tolower);

    if (core::array{".png", ".bmp", ".tga", ".jpg"} / core::count_if([&](auto s) { return s == extension; }) == 0)
        return  {std::runtime_error("Can't save image '" + file_path + "': unsupported extension '" + extension + "'")};

    extension = extension.substr(1);

    auto bytes = save_color_map_to_bytes(color_map, extension);
    if (bytes) {
        if (core::write_file(file_path, *bytes))
            return true;
        else
            return {std::runtime_error("Can't create file \"" + file_path + "\"")};
    } else
        return bytes.exception_ptr();
}

/**
 * @brief Saves the color map to file
 *
 * @throw exception if color map can't be saved
 *
 * @tparam T - the type of the color component
 * @tparam S - the color channels count
 * @param color_map - the color map to be saved
 * @param file_path - a path to new file with extension
 */
template <typename T, size_t S>
void save_color_map_unwrap(const grx_color_map<T, S>& color_map, const core::string& file_path) {
    save_color_map(color_map, file_path).value();
}

/**
 * @brief Saves the color map to file asynchronously
 *
 * @tparam T - the type of the color component
 * @tparam S - the color channels count
 * @param color_map - the color map to be saved
 * @param file_path - a path to new file with extension
 *
 * @return the future with failure_opt with bool value if save successfull or exception_ptr
 */
template <typename T, size_t S>
[[nodiscard]]
core::job_future<core::failure_opt<bool>>
save_color_map_async(const grx_color_map<T, S>& color_map, const core::string& file_path) {
    return core::job_future(save_color_map<T, S>, color_map, file_path);
}
} // namespace grx


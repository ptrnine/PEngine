#pragma once

#include <ostream>

extern "C" {
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
}

#include <core/types.hpp>
#include <core/container_extensions.hpp>
#include <core/vec.hpp>
#include <core/math.hpp>
#include <core/async.hpp>
#include "grx_types.hpp"

namespace grx {
    using core::vec;
    using core::vec2u;
    using core::zip_view;
    using core::FloatingPoint;

    template <typename T, size_t NPP, bool Const = false>
    class grx_color_pixel_view {
    public:
        grx_color_pixel_view(const grx_color_pixel_view& v): _data(v._data) {}

        grx_color_pixel_view& operator=(const grx_color_pixel_view& v) {
            _data = v._data;
        }

        template <typename TT> requires (!Const)
        grx_color_pixel_view& operator=(const vec<TT, NPP>& color) {
            vec<T, NPP> result_color;

            if constexpr ((FloatingPoint<TT> && FloatingPoint<T>) || (!FloatingPoint<TT> && !FloatingPoint<T>))
                result_color = static_cast<vec<T, NPP>>(color);
            else if constexpr (!FloatingPoint<TT> && FloatingPoint<T>)
                result_color = static_cast<vec<T, NPP>>(color) / T(255);
            else if constexpr (FloatingPoint<TT> && !FloatingPoint<T>)
                result_color = static_cast<vec<T, NPP>>(core::round(color * TT(255)));

            memcpy(_data, result_color.v.data(), NPP * sizeof(T));
            return *this;
        }

        vec<T, NPP> get() const {
            vec<T, NPP> color;
            memcpy(color.v.data(), _data, NPP * sizeof(T));
            return color;
        }

        void print(std::ostream& os) const {
            core::string str = "#";
            vec<uint8_t, NPP> color;

            if constexpr (!FloatingPoint<T>)
                color = static_cast<vec<uint8_t, NPP>>(get());
            else
                color = static_cast<vec<uint8_t, NPP>>(
                    core::round(static_cast<vec<T, NPP>>(get()) * T(255)));

            for (auto c : color.v) {
                str.push_back("0123456789abcdef"[c >> 4]);
                str.push_back("0123456789abcdef"[c & 0x0f]);
            }

            os << str;
        }

    private:
        template <typename, size_t, bool>
        friend class grx_color_row_view;

        template <typename, size_t, bool>
        friend class grx_color_map_iterator;

        grx_color_pixel_view(T* data): _data(data) {}

    private:
        T* _data;
    };


    template <typename T, size_t NPP, bool Const = false>
    class grx_color_map_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using difference_type   = std::ptrdiff_t;

        grx_color_map_iterator() = default;

        grx_color_map_iterator(const grx_color_map_iterator<T, NPP, true>& it): _ptr(it._ptr) {}

        grx_color_map_iterator& operator++() {
            _ptr += NPP;
            return *this;
        }

        grx_color_map_iterator& operator--() {
            _ptr -= NPP;
            return *this;
        }

        grx_color_map_iterator operator++(int) {
            auto res = *this;
            _ptr += NPP;
            return res;
        }

        grx_color_map_iterator operator--(int) {
            auto res = *this;
            _ptr -= NPP;
            return res;
        }

        grx_color_map_iterator& operator+=(difference_type n) {
            _ptr += n * NPP;
            return *this;
        }

        grx_color_map_iterator& operator-=(difference_type n) {
            _ptr -= n * NPP;
            return *this;
        }

        grx_color_map_iterator operator+(difference_type n) const {
            auto res = *this;
            res += n;
            return res;
        }

        grx_color_map_iterator operator-(difference_type n) const {
            auto res = *this;
            res -= n;
            return res;
        }

        difference_type operator-(const grx_color_map_iterator& it) const {
            return (_ptr - it._ptr) / NPP;
        }

        bool operator<(const grx_color_map_iterator& it) const {
            return _ptr < it._ptr;
        }

        bool operator>(const grx_color_map_iterator& it) const {
            return _ptr > it._ptr;
        }

        bool operator==(const grx_color_map_iterator& it) const {
            return _ptr == it._ptr;
        }

        bool operator>=(const grx_color_map_iterator& it) const {
            return !(*this < it);
        }

        bool operator<=(const grx_color_map_iterator& it) const {
            return !(*this > it);
        }

        bool operator!=(const grx_color_map_iterator& it) const {
            return !(*this == it);
        }

        grx_color_pixel_view<T, NPP, Const> operator*() const {
            return grx_color_pixel_view<T, NPP, Const>(_ptr);
        }

        grx_color_pixel_view<T, NPP, Const> operator[](difference_type n) const {
            return *(*this + n);
        }

    private:
        template <typename, size_t>
        friend class grx_color_map;

        grx_color_map_iterator(T* ptr): _ptr(ptr) {}

    private:
        T* _ptr = nullptr;
    };


    template <typename T, size_t NPP, bool Const = false>
    class grx_color_row_view {
    public:
        vec<T, NPP> operator[](size_t pos) const {
            vec<T, NPP> color;
            memcpy(color.v.data(), _data + pos * NPP, NPP * sizeof(T));
            return color;
        }

        grx_color_pixel_view<T, NPP, Const> at(size_t pos) const {
            return grx_color_pixel_view<T, NPP, Const>(_data + pos * NPP);
        }

        grx_color_map_iterator<T, NPP, Const>
        begin() const {
            return grx_color_map_iterator<T, NPP, Const>(_data);
        }

        grx_color_map_iterator<T, NPP, Const>
        end() const {
            return grx_color_map_iterator<T, NPP, Const>(_data + _width * NPP);
        }


    private:
        template <typename, size_t>
        friend class grx_color_map;

        grx_color_row_view(T* data, uint width): _data(data), _width(width) {}
        grx_color_row_view(const grx_color_row_view&) = delete;
        grx_color_row_view& operator=(const grx_color_row_view&) = delete;

    private:
        T*   _data;
        uint _width;
    };


    template <typename T, size_t NPP>
    class grx_color_map {
    public:
        grx_color_map(T* raw_data, const vec2u& size): _size(size) {
            _data = new T[_size.x() * _size.y() * NPP];
            memcpy(_data, raw_data, _size.x() * _size.y() * NPP * sizeof(T));
        }

        grx_color_map(const vec2u& size): _size(size) {
            _data = new T[_size.x() * _size.y() * NPP];
        }

        grx_color_map(const grx_color_map& map): grx_color_map(map._data, map._size) {}

        grx_color_map& operator=(const grx_color_map& map) {
            if (_data)
                delete [] _data;

            _size = map._size;
            _data = new T[_size.x() * _size.y() * NPP];
            memcpy(_data, map._data, _size.x() * _size.y() * NPP * sizeof(T));

            return *this;
        }

        grx_color_map(grx_color_map&& map) noexcept: _size(map._size), _data(map._data) {
            map._data = nullptr;
        }

        grx_color_map& operator=(grx_color_map&& map) noexcept {
            if (_data)
                delete [] _data;

            _size = map._size;
            _data = map._data;
            map._data = nullptr;

            return *this;
        }

        ~grx_color_map() {
            if (_data)
                delete [] _data;
        }

        grx_color_map_iterator<T, NPP>
        begin() {
            return grx_color_map_iterator<T, NPP>(_data);
        }

        grx_color_map_iterator<T, NPP>
        end() {
            return grx_color_map_iterator<T, NPP>(_data + _size.x() * _size.y() * NPP);
        }

        grx_color_map_iterator<T, NPP, true>
        begin() const {
            return grx_color_map_iterator<T, NPP, true>(_data);
        }

        grx_color_map_iterator<T, NPP, true>
        end() const {
            return grx_color_map_iterator<T, NPP, true>(_data + _size.x() * _size.y() * NPP);
        }

        grx_color_row_view<T, NPP, true>
        operator[](size_t row_num) const {
            return grx_color_row_view<T, NPP, true>(_data + row_num * _size.x() * NPP, _size.x());
        }

        grx_color_row_view<T, NPP>
        operator[](size_t row_num) {
            return grx_color_row_view<T, NPP>(_data + row_num * _size.x() * NPP, _size.x());
        }

        template <typename TT>
        explicit operator grx_color_map<TT, NPP>() const {
            grx_color_map<TT, NPP> result(_size);

            for (auto& [dst, src] : zip_view(result, *this))
                dst = src.get();

            return result;
        }

        const vec2u& size() const {
            return _size;
        }

        const T* data() const {
            return _data;
        }

        T* data() {
            return _data;
        }

    private:
        vec2u _size;
        T*    _data = nullptr;
    };

    using grx_color_map_r    = grx_color_map<uint8_t, 1>;
    using grx_color_map_rg   = grx_color_map<uint8_t, 2>;
    using grx_color_map_rgb  = grx_color_map<uint8_t, 3>;
    using grx_color_map_rgba = grx_color_map<uint8_t, 4>;

    using grx_float_color_map_r    = grx_color_map<float, 1>;
    using grx_float_color_map_rg   = grx_color_map<float, 2>;
    using grx_float_color_map_rgb  = grx_color_map<float, 3>;
    using grx_float_color_map_rgba = grx_color_map<float, 4>;


    template <core::MathVector T = color_rgb>
    grx_color_map<typename T::value_type, T::size()>
    load_color_map_from_bytes(core::span<core::byte> bytes) {
        static_assert(T::size() < 5 && T::size() > 0,
                      "Wrong color type. T must be uint8_t or float, Size must be 1 <= Size <= 4");

        int w, h, comp;
        int type;

        if constexpr (T::size() == 1)
            type = STBI_grey;
        else if constexpr (T::size() == 2)
            type = STBI_grey_alpha;
        else if constexpr (T::size() == 3)
            type = STBI_rgb;
        else if constexpr (T::size() == 4)
            type = STBI_rgb_alpha;

        auto img = stbi_load_from_memory(
                reinterpret_cast<const uint8_t*>(bytes.data()),
                static_cast<int>(bytes.size()),
                &w,
                &h,
                &comp,
                type);

        if (!img)
            throw std::runtime_error("Can't load image from memory");

        auto map = grx_color_map<uint8_t, T::size()>(img, static_cast<vec2u>(vec{w, h}));
        stbi_image_free(img);

        if constexpr (std::is_same_v<typename T::value_type, uint8_t>)
            return map;
        else
            return static_cast<grx_color_map<typename T::value_type, T::size()>>(map);
    }


    template <core::MathVector T = color_rgb>
    [[nodiscard]]
    core::optional<grx_color_map<typename T::value_type, T::size()>>
    load_color_map(const core::string& file_path) {
        if (auto file = core::read_binary_file(core::path_eval(file_path)))
            return load_color_map_from_bytes<T>(*file);
        else
            return core::nullopt;
    }

    template <core::MathVector T = color_rgb>
    [[nodiscard]]
    grx_color_map<typename T::value_type, T::size()>
    load_color_map_unwrap(const core::string& file_path) {
        if (auto map = load_color_map<T>(file_path))
            return *map;
        else
            throw std::runtime_error("Can't load image '" + file_path + "'");
    }


    template <core::MathVector T = color_rgb>
    [[nodiscard]]
    std::future<core::optional<grx_color_map<typename T::value_type, T::size()>>>
    load_color_map_async(const core::string& file_path) {
        return std::async(std::launch::async, load_color_map<T>, file_path);
    }

    template <core::MathVector T = color_rgb>
    [[nodiscard]]
    std::future<grx_color_map<typename T::value_type, T::size()>>
    load_color_map_async_unwrap(const core::string& file_path) {
        return std::async(std::launch::async, load_color_map_unwrap<T>, file_path);
    }


    namespace color_map_save_bytes_helper {
        inline void write_callback(void* byte_vector, void* data, int size) {
            auto vec      = reinterpret_cast<core::vector<core::byte>*>(byte_vector);
            auto old_size = vec->size();
            vec->resize(old_size + static_cast<size_t>(size));
            memcpy(vec->data() + old_size, data, static_cast<size_t>(size));
        }
    }

    /**
     * Returns std::nullopt if fails
     * Throws exception if 'extension' is unsupported
     */
    template <typename T, size_t S>
    [[nodiscard]]
    core::optional<core::vector<core::byte>>
    save_color_map_to_bytes(const grx_color_map<T, S>& color_map, core::string_view extension = "png") {
        core::vector<core::byte> result;

        using save_func_t = std::function<int(stbi_write_func*, void*, int, int, int, const void*)>;
        save_func_t save_func;

        if (extension == "png")
            save_func = [&](stbi_write_func* f, void* c, int w, int h, int comp, const void* data) {
                return stbi_write_png_to_func(f, c, w, h, comp, data, static_cast<int>(color_map.size().x() * S)); };
        else if (extension == "bmp")
            save_func = stbi_write_bmp_to_func;
        else if (extension == "tga")
            save_func = stbi_write_tga_to_func;
        else if (extension == "jpg")
            save_func = [](stbi_write_func* f, void* c, int w, int h, int comp, const void* data) {
                return stbi_write_jpg_to_func(f, c, w, h, comp, data, 95); };
        else
            throw std::runtime_error("Can't save image: unsupported extension '" + core::string(extension) + "'");

        int rc;
        if constexpr (std::is_same_v<T, uint8_t>)
            rc = save_func(
                    color_map_save_bytes_helper::write_callback,
                    &result,
                    static_cast<int>(color_map.size().x()),
                    static_cast<int>(color_map.size().y()),
                    static_cast<int>(S),
                    color_map.data());
        else {
            auto map = static_cast<grx_color_map<uint8_t, S>>(color_map);
            rc = save_func(
                    color_map_save_bytes_helper::write_callback,
                    &result,
                    static_cast<int>(map.size().x()),
                    static_cast<int>(map.size().y()),
                    static_cast<int>(S),
                    map.data());
        }

        if (rc == 0)
            return core::nullopt;
        else
            return result;
    }

    template <typename T, size_t S>
    [[nodiscard]]
    core::vector<core::byte>
    save_color_map_to_bytes_unwrap(const grx_color_map<T, S>& color_map, core::string_view extension = "png") {
        if (auto result = save_color_map_to_bytes(color_map, extension))
            return *result;
        else
            throw std::runtime_error("Can't save image");
    }

    /**
     * Returns true on success
     * Throws exception if extension is unsupported or missed
     */
    template <typename T, size_t S>
    [[nodiscard]]
    bool save_color_map(const grx_color_map<T, S>& color_map, const core::string& file_path) {
        if (file_path.size() < 5)
            throw std::runtime_error("Can't save image '" + file_path + "': can't determine file format");

        auto extension = file_path.substr(file_path.size() - 4) /
            core::transform<core::string>(::tolower);

        if (core::array{".png", ".bmp", ".tga", ".jpg"} /
                core::count_if([&](auto s) { return s == extension; }) == 0)
            throw std::runtime_error("Can't save image '" + file_path + "': unsupported extension '" + extension + "'");

        extension = extension.substr(1);

        if (auto bytes = std::move(save_color_map_to_bytes(color_map, extension)))
            return core::write_file(file_path, *bytes);
        else
            return false;
    }

    template <typename T, size_t S>
    void save_color_map_unwrap(const grx_color_map<T, S>& color_map, const core::string& file_path) {
        if (!save_color_map(color_map, file_path))
            throw std::runtime_error("Can't save image '" + file_path + "'");
    }

    template <typename T, size_t S>
    [[nodiscard]]
    std::future<bool>
    save_color_map_async(const grx_color_map<T, S>& color_map, const core::string& file_path) {
        return std::async(std::launch::async, save_color_map<T, S>, color_map, file_path);
    }
} // namespace grx


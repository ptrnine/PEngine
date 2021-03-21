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
#include <core/files.hpp>
#include <core/math.hpp>
#include <core/types.hpp>
#include <core/vec.hpp>
#include <core/serialization.hpp>
#include <core/assert.hpp>
#include <util/compression.hpp>
#include <util/vtf_format.hpp>

namespace grx
{
using core::FloatingPoint;
using core::vec;
using core::vec2u;
using core::zip_view;
using core::u8;
using core::make_unique;

constexpr u8 COLOR_VAL_MAX = 255;
constexpr core::u32 MIPMAPS_COUNT = 8;
constexpr auto MIPMAP_MIN_SIZE = core::vec{2U, 2U};

/**
 * @brief Provides access to the pixel data
 *
 * @tparam T - the type of the color channel value
 * @tparam NPP - channels count
 * @tparam Const - const flag
 */
template <ColorComponent T, size_t NPP, bool Const = false>
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
    template <ColorComponent TT> requires(!Const) && ((core::Unsigned<TT> && core::Unsigned<T>) || std::same_as<T, TT>)
    grx_color_pixel_view& operator=(const vec<TT, NPP>& color) noexcept {
        auto result_color = static_cast<vec<T, NPP>>(color);
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
        if constexpr (!FloatingPoint<T>) {
            core::string str = "#";
            auto color = static_cast<vec<uint8_t, NPP>>(get());
            for (auto c : color.v) {
                str.push_back("0123456789abcdef"[c >> 4]);
                str.push_back("0123456789abcdef"[c & 0x0f]); // NOLINT
            }
            os << str;
        }
        else {
            os << core::format("{}", get());
        }
    }

private:
    template <ColorComponent, size_t, bool>
    friend class grx_color_row_view;

    template <ColorComponent, size_t, bool>
    friend class grx_color_map_iterator;

    grx_color_pixel_view(T* data) noexcept: _data(data) {}

private:
    T* _data;
};


template <ColorComponent T, size_t NPP, bool Const = false>
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
    template <ColorComponent, size_t>
    friend class grx_color_map;

    template <ColorComponent, size_t, bool>
    friend class grx_color_map_view;

    template <ColorComponent, size_t, bool>
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
template <ColorComponent T, size_t NPP, bool Const = false>
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
    template <ColorComponent, size_t>
    friend class grx_color_map;

    template <ColorComponent, size_t, bool>
    friend class grx_color_map_view;

    grx_color_row_view(T* data, uint width) noexcept: _data(data), _width(width) {}

private:
    T*   _data;
    uint _width;
};

template <ColorComponent T, size_t NPP, bool Const>
class grx_color_map_view {
public:
    grx_color_map_view(T* data, const vec2u& size): _data(data), _size(size) {}

    [[nodiscard]]
    const vec2u& size() const noexcept {
        return _size;
    }

    [[nodiscard]]
    size_t components_count() const noexcept {
        return _size.x() * _size.y() * NPP;
    }

    [[nodiscard]]
    T* data() const noexcept {
        return _data;
    }

    grx_color_map_iterator<T, NPP, Const> begin() const noexcept {
        return grx_color_map_iterator<T, NPP, Const>(_data);
    }

    grx_color_map_iterator<T, NPP, Const> end() const noexcept {
        return grx_color_map_iterator<T, NPP, Const>(_data + _size.x() * _size.y() * NPP);
    }

    grx_color_row_view<T, NPP, Const> operator[](size_t row_num) const noexcept {
        return grx_color_row_view<T, NPP, Const>(_data + row_num * _size.x() * NPP, _size.x());
    }

    template <typename IdxT>
    grx_color_pixel_view<T, NPP, Const> operator[](vec<IdxT, 2> pos) const noexcept {
        return (*this)[pos.y()][pos.x()];
    }

    void flip_horizontal() {
        auto half_size = vec2u{_size.x() / 2, _size.y()};
        for (auto [x, y] : core::dimensional_seq(half_size)) {
            auto tmp_color                = (*this)[y][x].get();
            (*this)[y][x]                 = (*this)[y][_size.x() - x - 1].get();
            (*this)[y][_size.x() - x - 1] = tmp_color;
        }
    }

    void flip_vertical() {
        auto half_size = vec2u{_size.x(), _size.y() / 2};
        for (auto [x, y] : core::dimensional_seq(half_size)) {
            auto tmp_color                = (*this)[y][x].get();
            (*this)[y][x]                 = (*this)[_size.y() - y - 1][x].get();
            (*this)[_size.y() - y - 1][x] = tmp_color;
        }
    }

private:
    T*    _data;
    vec2u _size;
};

template <ColorComponent T, size_t NPP, bool Const>
class grx_color_map_level_view {
public:
    class grx_color_map_level_iterator {
    public:
        friend class grx_color_map_level_view;
        using iterator_category = std::bidirectional_iterator_tag;

        static core::pair<vec2u, size_t> skip_compcount(vec2u size, uint mipmap) {
            size_t skip = 0;
            for ([[maybe_unused]] uint i = 0; i < mipmap; ++i) {
                skip += size.x() * size.y() * NPP;
                size /= 2U;
                if (size.x() < MIPMAP_MIN_SIZE.x())
                    size.x() = MIPMAP_MIN_SIZE.x();
                if (size.y() < MIPMAP_MIN_SIZE.y())
                    size.y() = MIPMAP_MIN_SIZE.y();
            }
            return {size, skip};
        }

        grx_color_map_level_iterator& operator++() noexcept {
            if (_mipmap < _max_mipmaps)
                ++_mipmap;
            return *this;
        }

        grx_color_map_level_iterator& operator--() noexcept {
            if (_mipmap != 0)
                --_mipmap;
            return *this;
        }

        grx_color_map_level_iterator operator++(int) noexcept {
            auto res = *this;
            ++(*this);
            return res;
        }

        grx_color_map_level_iterator operator--(int) noexcept {
            auto res = *this;
            --(*this);
            return res;
        }

        bool operator==(const grx_color_map_level_iterator& it) const noexcept {
            return _mipmap == it._mipmap;
        }
        bool operator!=(const grx_color_map_level_iterator& it) const noexcept {
            return !(*this == it);
        }

        grx_color_map_view<T, NPP, Const> operator*() const noexcept {
            auto [size, skip] = skip_compcount(_size, _mipmap);
            return grx_color_map_view<T, NPP, Const>(_data + skip, size);
        }

    private:
        grx_color_map_level_iterator(const vec2u& size, uint mipmap_count, T* data, uint mipmap = 0):
            _size(size), _max_mipmaps(mipmap_count + 1), _data(data), _mipmap(mipmap) {}

        vec2u _size;
        uint  _max_mipmaps;
        T*    _data;
        uint  _mipmap = 0;
    };

    grx_color_map_level_view(T* data, const vec2u& size, uint mipmap_count):
        _begin(size, mipmap_count, data), _end(size, mipmap_count, data, mipmap_count) {}

    grx_color_map_level_iterator begin() const noexcept {
        return _begin;
    }

    grx_color_map_level_iterator end() const noexcept {
        return _end;
    }

private:
    grx_color_map_level_iterator _begin;
    grx_color_map_level_iterator _end;
};

template <ColorComponent T, size_t NPP>
class grx_color_map {
public:
    using c_array = T[]; // NOLINT

    static size_t calc_compcount(const vec2u& size, core::u32 mipmaps) {
        size_t compcount = size.x() * size.y() * NPP;
        auto mipmap_size = size / 2U;

        for (core::u32 i = 0; i < mipmaps; ++i) {
            compcount += mipmap_size.x() * mipmap_size.y() * NPP;
            mipmap_size /= 2U;
            if (mipmap_size.x() < MIPMAP_MIN_SIZE.x())
                mipmap_size.x() = MIPMAP_MIN_SIZE.x();
            if (mipmap_size.y() == MIPMAP_MIN_SIZE.y())
                mipmap_size.y() = MIPMAP_MIN_SIZE.y();
        }

        return compcount;
    }

    grx_color_map(T* raw_data, const vec2u& size, core::u32 mipmaps = 0):
        _size(size), _mipmaps(mipmaps) {
        if (mipmaps && (!core::is_power_of_two(size.x()) || !core::is_power_of_two(size.y()))) {
            auto newsize = core::vec{core::next_power_of_two(_size.x()), core::next_power_of_two(_size.y())};
            LOG_WARNING("Sizes for mipmaping must be power of two: change size from {} to {}", _size, newsize);
            _size = newsize;
        }

        _compcount = calc_compcount(_size, _mipmaps);
        _data = make_unique<c_array>(_compcount);
        memcpy(_data.get(), raw_data, _compcount * sizeof(T));
    }

    grx_color_map(const vec2u& size, core::u32 mipmaps = 0): _size(size), _mipmaps(mipmaps) {
        if (mipmaps && (!core::is_power_of_two(size.x()) || !core::is_power_of_two(size.y()))) {
            auto newsize = core::vec{core::next_power_of_two(_size.x()), core::next_power_of_two(_size.y())};
            LOG_WARNING("Sizes for mipmaping must be power of two: change size from {} to {}", _size, newsize);
            _size = newsize;
        }

        _compcount = calc_compcount(_size, _mipmaps);
        _data = make_unique<c_array>(_compcount);
    }

    grx_color_map(const grx_color_map& map):
        grx_color_map(map._data.get(), map._size, map._mipmaps) {}

    grx_color_map& operator=(const grx_color_map& map) {
        _size      = map._size;
        _compcount = map._compcount;
        _mipmaps   = map._mipmaps;
        _data      = make_unique<c_array>(_compcount);
        memcpy(_data.get(), map._data.get(), _compcount * sizeof(T));

        return *this;
    }

    grx_color_map(grx_color_map&& map) noexcept:
        _size(map._size),
        _data(std::move(map._data)),
        _compcount(map._compcount),
        _mipmaps(map._mipmaps) {
        map._data = nullptr;
    }

    grx_color_map& operator=(grx_color_map&& map) noexcept {
        _size     = map._size;
        _data     = std::move(map._data);
        _mipmaps  = map._mipmaps;
        _compcount = map._compcount;
        map._data = nullptr;

        return *this;
    }

    ~grx_color_map() noexcept = default;

    static grx_color_map from_bytes(core::span<core::byte> bytes) {
        if constexpr (std::endian::native == std::endian::big)
            pe_throw std::runtime_error("Implement me for big endian");

        using namespace core;

        deserializer_view ds{bytes};
        ds.read_get<array<char, 4>>();

        auto channels = ds.read_get<u32>();
        PeRelRequireF(channels == NPP, "Invalid channels count: gets {} but {} required", channels, NPP);

        auto size = ds.read_get<vec<u32, 2>>();
        auto mipmaps = ds.read_get<u32>();

        if (mipmaps && (!core::is_power_of_two(size.x()) || !core::is_power_of_two(size.y())))
            pe_throw std::runtime_error("Sizes must be power of two");

        ds.read_get<u64>(); /* Skip compressed size */

        auto compcount      = calc_compcount(size, mipmaps);
        auto data           = make_unique<c_array>(compcount);
        auto img_compressed = bytes.subspan(28);
        auto img_data       = util::decompress_block(img_compressed, compcount * sizeof(T));

        PeRelRequireF(compcount == static_cast<size_t>(img_data.size()),
                      "Invalid image size: gets {} but {} required",
                      img_data.size(),
                      compcount);
        memcpy(data.get(), img_data.data(), compcount * sizeof(T));

        grx_color_map result;
        result._size = size;
        result._data = move(data);
        result._mipmaps = mipmaps;
        result._compcount = compcount;

        return result;
    }

    [[nodiscard]]
    core::vector<core::byte> to_bytes() const {
        if constexpr (std::endian::native == std::endian::big)
            pe_throw std::runtime_error("Implement me for big endian");

        using namespace core;

        serializer s;
        s.write(array{'P', 'E', 'T', 'X'});
        s.write(static_cast<u32>(NPP));
        s.write(static_cast<vec<u32, 2>>(_size));
        s.write(_mipmaps);
        s.write(util::compress(span{reinterpret_cast<core::byte*>(_data.get()), // NOLINT
                                    static_cast<ssize_t>(_compcount)},
                               util::COMPRESS_FAST));

        return s.detach_data();
    }

    void gen_mipmaps() {
        if (!core::is_power_of_two(_size.x()) || !core::is_power_of_two(_size.y()))
            pe_throw std::runtime_error("Sizes must be power of two");

        auto compcount = calc_compcount(_size, MIPMAPS_COUNT);
        auto newdata = make_unique<c_array>(compcount);
        auto ptr = newdata.get();

        auto src_compcount = _size.x() * _size.y() * NPP;
        std::memcpy(ptr, _data.get(), src_compcount * sizeof(T));

        auto   mipmap_size  = _size;
        size_t src_displacement = 0;
        for (core::u32 i = 0; i < MIPMAPS_COUNT; ++i) {
            auto new_mipmap_size = mipmap_size / 2U;
            if (mipmap_size.x() < MIPMAP_MIN_SIZE.x())
                mipmap_size.x() = MIPMAP_MIN_SIZE.x();
            if (mipmap_size.y() == MIPMAP_MIN_SIZE.y())
                mipmap_size.y() = MIPMAP_MIN_SIZE.y();

            if constexpr (core::FloatingPoint<T>)
                stbir_resize_float(ptr + src_displacement,
                                   static_cast<int>(mipmap_size.x()),
                                   static_cast<int>(mipmap_size.y()),
                                   0,
                                   ptr + src_displacement + src_compcount,
                                   static_cast<int>(new_mipmap_size.x()),
                                   static_cast<int>(new_mipmap_size.y()),
                                   0,
                                   static_cast<int>(NPP));
            else
                stbir_resize_uint8(ptr + src_displacement,
                                   static_cast<int>(mipmap_size.x()),
                                   static_cast<int>(mipmap_size.y()),
                                   0,
                                   ptr + src_displacement + src_compcount,
                                   static_cast<int>(new_mipmap_size.x()),
                                   static_cast<int>(new_mipmap_size.y()),
                                   0,
                                   static_cast<int>(NPP));

            auto dst_compcount = new_mipmap_size.x() * new_mipmap_size.y() * NPP;

            src_displacement += src_compcount;
            src_compcount = dst_compcount;
            mipmap_size = new_mipmap_size;
        }

        _data = move(newdata);
        _mipmaps = MIPMAPS_COUNT;
        _compcount = compcount;
    }

    grx_color_map_level_view<T, NPP, false> mipmap_view() {
        return grx_color_map_level_view<T, NPP, false>(_data.get(), _size, _mipmaps);
    }

    grx_color_map_level_view<T, NPP, true> mipmap_view() const {
        return grx_color_map_level_view<T, NPP, true>(_data.get(), _size, _mipmaps);
    }

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

    /*
    template <typename TT>
    [[nodiscard]]
    explicit operator grx_color_map<TT, NPP>() const {
        grx_color_map<TT, NPP> result(_size);

        for (auto& [dst, src] : zip_view(result, *this))
            dst = src.get();

        return result;
    }
    */

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

    [[nodiscard]]
    bool has_mipmaps() const noexcept {
        return _mipmaps > 0;
    }

    [[nodiscard]]
    core::u32 mipmaps_count() const noexcept {
        return _mipmaps;
    }

    [[nodiscard]]
    size_t components_count() const noexcept {
        return _compcount;
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
        result._compcount = new_size.x() * new_size.y() * NPP;

        return result;
    }

    template <bool Enable = std::is_floating_point_v<T>>
    auto to_ldr(float gamma = 1.f, float exposure = 1.f) const
        -> std::enable_if_t<Enable, grx_color_map<uint8_t, NPP>> {
        auto result = grx_color_map<uint8_t, NPP>(_size, _mipmaps);
        for (size_t i = 0; i < _compcount; ++i) {
            auto mapped = T(1.0) - std::exp(-_data[i] * exposure);
            result.data()[i] = static_cast<uint8_t>(T(255.0) * std::pow(mapped, T(1.0) / gamma));
        }
        return result;
    }

    template <bool Enable = !std::is_floating_point_v<T>>
    auto to_hdr() const -> std::enable_if_t<Enable, grx_color_map<float, NPP>> {
        auto result = grx_color_map<float, NPP>(_size, _mipmaps);
        for (size_t i = 0; i < _compcount; ++i)
            result.data()[i] = static_cast<float>(_data[i]) / 255.f;
        return result;
    }

    void flip_horizontal() {
        for (auto cmap_view : mipmap_view())
            cmap_view.flip_horizontal();
    }

    void flip_vertical() {
        for (auto cmap_view : mipmap_view())
            cmap_view.flip_vertical();
    }

    grx_color_map flipped_horizontal() const {
        auto res = *this;
        res.flip_horizontal();
        return res;
    }

    grx_color_map flipped_vertical() const {
        auto res = *this;
        res.flip_vertical();
        return res;
    }

    grx_color_map rotated_left() const {
        auto res = grx_color_map(_size.yx(), _mipmaps);
        auto src_view = mipmap_view();
        auto dst_view = res.mipmap_view();
        for (auto [dst, src] : core::zip_view(dst_view, src_view))
            for (auto [x, y] : core::dimensional_seq(src.size()))
                dst[src.size().x() - x - 1][y] = src[y][x].get();

        return res;
    }

    grx_color_map rotated_right() const {
        auto res = grx_color_map(_size.yx(), _mipmaps);
        auto src_view = mipmap_view();
        auto dst_view = res.mipmap_view();
        for (auto [dst, src] : core::zip_view(dst_view, src_view))
            for (auto [x, y] : core::dimensional_seq(src.size()))
                dst[x][src.size().y() - y - 1] = src[y][x].get();

        return res;
    }

    grx_color_map rotated_180() const {
        grx_color_map res = grx_color_map(_size, _mipmaps);

        auto src_view = mipmap_view();
        auto dst_view = res.mipmap_view();
        for (auto [dst, src] : core::zip_view(dst_view, src_view))
            for (auto [x, y] : core::dimensional_seq(src.size()))
                dst[src.size().y() - y - 1][src.size().x() - x - 1] = src[y][x].get();

        return res;
    }

    grx_color_map mirrored_down() const {
        auto new_size = vec2u{_size.x(), _size.y() * 2U};
        auto res = grx_color_map(new_size, _mipmaps);
        auto dst_view = res.mipmap_view();
        auto src_view = mipmap_view();

        for (auto [dst, src] : core::zip_view(dst_view, src_view)) {
            auto compcount = src.components_count();
            std::memcpy(dst.data(), src.data(), compcount * sizeof(T));
            for (auto [x, y] : core::dimensional_seq(src.size()))
                dst[y + src.size().y()][x] = src[src.size().y() - y - 1][x].get();
        }

        return res;
    }

private:
    template <ColorComponent, size_t>
    friend class grx_texture;

    grx_color_map() noexcept = default;

private:
    vec2u                _size = {0, 0};
    std::unique_ptr<T[]> _data = nullptr; // NOLINT
    size_t               _compcount = 0;
    core::u32            _mipmaps = 0;
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
 * @return the color map or exception in try_opt
 */
template <core::MathVector T = color_rgb>
core::try_opt<grx_color_map<typename T::value_type, T::size()>>
try_load_color_map_from_bytes(core::span<core::byte> bytes) {
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

    if (bytes.size() > 4 && core::deserializer_view{bytes}.read_get<core::array<char, 4>>() ==
                                core::array{'P', 'E', 'T', 'X'}) {
        if constexpr (std::is_same_v<typename T::value_type, uint8_t>)
            return grx_color_map<uint8_t, T::size()>::from_bytes(bytes);
        else
            return grx_color_map<uint8_t, T::size()>::from_bytes(bytes).to_hdr();
    }

    auto img = stbi_load_from_memory(reinterpret_cast<const uint8_t*>(bytes.data()), // NOLINT
                                     static_cast<int>(bytes.size()),
                                     &w,
                                     &h,
                                     &comp,
                                     type);
    auto scope_exit = core::scope_guard([img]{ if (img) stbi_image_free(img); });

    if (!img) {
        auto vtf = util::vtf_view(bytes);
        if (!vtf.is_valid())
            return {std::runtime_error("Can't load image from memory")};

        if (vtf.is_high_res_hdr() && !std::is_same_v<typename T::value_type, float>)
            return {std::runtime_error("Can't load HDR vtf texture to non-HDR color map")};

        if (vtf.high_res_channels_count() != T::size())
            return {std::runtime_error(core::format("Wrong channels count: vtf texture has {} "
                                                    "channels, but color map require {} channes",
                                                    vtf.high_res_channels_count(),
                                                    T::size()))};

        if constexpr (std::is_same_v<typename T::value_type, float>) {
            if (!vtf.is_high_res_hdr()) {
                auto map = grx_color_map<uint8_t, T::size()>(static_cast<vec2u>(vtf.high_res_size()));
                vtf.read_high_res(map.data());
                if (vtf.mipmap_count() > 1)
                    map.gen_mipmaps();
                return map.to_hdr();
            }
        }

        auto map = grx_color_map<typename T::value_type, T::size()>(static_cast<vec2u>(vtf.high_res_size()));
        vtf.read_high_res(map.data());
        /* TODO: do not regen mipmaps */
        if (vtf.mipmap_count() > 1)
            map.gen_mipmaps();
        return map;
    }

    auto map = grx_color_map<uint8_t, T::size()>(img, static_cast<vec2u>(vec{w, h}));

    if constexpr (std::is_same_v<typename T::value_type, uint8_t>)
        return map;
    else
        return map.to_hdr();
}

/**
 * @brief Loads a color map from the image file
 *
 * @tparam T - the color type
 * @param file_path - a path to the image file
 *
 * @return the read color map or exception in try_opt
 */
template <core::MathVector T = color_rgb>
[[nodiscard]]
core::try_opt<grx_color_map<typename T::value_type, T::size()>>
try_load_color_map(const core::string& file_path) {
    if (auto file = core::try_read_binary_file(core::path_eval(file_path)))
        return try_load_color_map_from_bytes<T>(*file);
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
load_color_map(const core::string& file_path) {
    return try_load_color_map<T>(file_path).value();
}

/**
 * @brief Loads a color map from the image file asynchronously
 *
 * @tparam T - the color type
 * @param file_path - a path to the image file
 *
 * @return the future to the color_map or exception in the try_opt
 */
template <core::MathVector T = color_rgb>
[[nodiscard]]
core::job_future<core::try_opt<grx_color_map<typename T::value_type, T::size()>>>
try_load_color_map_async(const core::string& file_path) {
    return core::submit_job(try_load_color_map<T>, file_path);
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
load_color_map_async(const core::string& file_path) {
    return core::submit_job(load_color_map<T>, file_path);
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
 * @return byte vector with image or exception in try_opt
 */
template <ColorComponent T, size_t S>
[[nodiscard]]
core::try_opt<core::vector<core::byte>>
try_save_color_map_to_bytes(const grx_color_map<T, S>& color_map, core::string_view extension = "png") {
    if (extension == "petx")
        return color_map.to_bytes();

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
        auto map = color_map.to_ldr();
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
template <ColorComponent T, size_t S>
[[nodiscard]]
core::vector<core::byte>
save_color_map_to_bytes_(const grx_color_map<T, S>& color_map, core::string_view extension = "png") {
    return try_save_color_map_to_bytes(color_map, extension).value();
}

/**
 * @brief Saves the color map to file
 *
 * @tparam T - the type of the color component
 * @tparam S - the color channels count
 * @param color_map - the color map to be saved
 * @param file_path - a path to new file with extension
 *
 * @return the try_opt with bool value if save successfull or exception_ptr
 */
template <ColorComponent T, size_t S>
[[nodiscard]]
core::try_opt<bool>
try_save_color_map(const grx_color_map<T, S>& color_map, const core::string& file_path) {
    constexpr size_t minimal_len = 5; // a.bmp, c.jpg, ...

    if (file_path.size() < minimal_len)
        return {std::runtime_error("Can't save image '" + file_path +
                                   "': can't determine file format")};

    auto dot_pos = file_path.rfind('.');
    if (dot_pos == core::string::npos)
        return std::runtime_error("Can't save color map '" + file_path + "': has no extension");
    auto extension = file_path.substr(dot_pos) / core::transform<core::string>(::tolower);

    if (core::array{".png", ".bmp", ".tga", ".jpg", ".petx"} /
            core::count_if([&](auto s) { return s == extension; }) ==
        0)
        return {std::runtime_error("Can't save image '" + file_path + "': unsupported extension '" +
                                   extension + "'")};

    extension = extension.substr(1);

    auto bytes = try_save_color_map_to_bytes(color_map, extension);
    if (bytes) {
        if (core::try_write_file(file_path, *bytes))
            return true;
        else
            return {std::runtime_error("Can't create file \"" + file_path + "\"")};
    }
    else
        return bytes.thrower_ptr();
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
template <ColorComponent T, size_t S>
void save_color_map(const grx_color_map<T, S>& color_map, const core::string& file_path) {
    try_save_color_map(color_map, file_path).value();
}

/**
 * @brief Saves the color map to file asynchronously
 *
 * @tparam T - the type of the color component
 * @tparam S - the color channels count
 * @param color_map - the color map to be saved
 * @param file_path - a path to new file with extension
 *
 * @return the future with try_opt with bool value if save successfull or exception_ptr
 */
template <ColorComponent T, size_t S>
[[nodiscard]]
core::job_future<core::try_opt<bool>>
try_save_color_map_async(const grx_color_map<T, S>& color_map, const core::string& file_path) {
    return core::job_future(try_save_color_map<T, S>, color_map, file_path);
}
} // namespace grx


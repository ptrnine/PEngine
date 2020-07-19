#include "container_extensions.hpp"
#include "core/helper_macros.hpp"
#include "types.hpp"
#include "vec.hpp"

namespace core
{
static constexpr auto _base64_map =
    array{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
          'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
          'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
          'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

namespace base64_helper
{
    template <size_t N = 0, size_t S = _base64_map.size()>
    inline constexpr size_t charcode_pos(size_t code) {
        if constexpr (N == S)
            return 0;
        else
            return static_cast<char>(code) == std::get<N>(_base64_map) ? N : charcode_pos<N + 1>(code);
    }

    template <size_t... Idxs>
    constexpr auto iota_vec(std::index_sequence<Idxs...>&&) {
        return vec{Idxs...};
    }

    template <size_t S>
    constexpr auto iota_vec() {
        return iota_vec(std::make_index_sequence<S>());
    }
} // namespace base64_helper

static constexpr auto _base64_dmap = vec_map(base64_helper::iota_vec<256>(), [](auto c) {
                                         return static_cast<uint8_t>(base64_helper::charcode_pos(c));
                                     }).v;

inline string base64_encode(const void* data, size_t size) {
    if (size == 0)
        return "";

    auto   p           = static_cast<const uint8_t*>(data);
    size_t padd        = (size * 4) % 3;
    size_t full_hexads = (size * 4) / 3;

    string result;
    result.resize(full_hexads + (padd == 1 ? 3 : (padd == 2 ? 2 : 0)));

    size_t bpos = 0;
    for (size_t i = 0; i < (size / 3) * 3; i += 3) {
        result[bpos++] = _base64_map[static_cast<size_t>(  p[i + 0] >> 2)];                                // NOLINT
        result[bpos++] = _base64_map[static_cast<size_t>(((p[i + 0] << 4) & 0b00111111) | p[i + 1] >> 4)]; // NOLINT
        result[bpos++] = _base64_map[static_cast<size_t>(((p[i + 1] << 2) & 0b00111111) | p[i + 2] >> 6)]; // NOLINT
        result[bpos++] = _base64_map[static_cast<size_t>(  p[i + 2]       & 0b00111111)];                  // NOLINT
    }

    if (padd == 2) {
        result[bpos++] = _base64_map[static_cast<size_t>(  p[size - 2] >> 2)];                                   // NOLINT
        result[bpos++] = _base64_map[static_cast<size_t>(((p[size - 2] << 4) & 0b00111111) | p[size - 1] >> 4)]; // NOLINT
        result[bpos++] = _base64_map[static_cast<size_t>(( p[size - 1] << 2) & 0b00111111)];                     // NOLINT
        result[bpos++] = '=';
    }
    else if (padd == 1) {
        result[bpos++] = _base64_map[static_cast<size_t>(p[size - 1] >> 2)];                // NOLINT
        result[bpos++] = _base64_map[static_cast<size_t>((p[size - 1] << 4) & 0b00111111)]; // NOLINT
        result[bpos++] = '=';
        result[bpos++] = '=';
    }

    return result;
}

inline size_t base64_byte_size(string_view base64) {
    if (base64.size() < 4)
        return 0;

    auto padd_count = std::count_if(base64.end() - 2, base64.end(), [](auto c) { return c == '='; });
    return (base64.size() / 4) * 3 - static_cast<size_t>(padd_count);
}

inline void base64_decode(void* dst, string_view base64) {
    if (base64.size() < 4)
        return;

    auto padd_count =
        static_cast<size_t>(std::count_if(base64.end() - 2, base64.end(), [](auto c) { return c == '='; }));
    auto full_hexads = ((base64.size() - padd_count) / 4) * 4;

    auto size   = base64_byte_size(base64);
    auto output = make_unique<uint8_t[]>(size); // NOLINT

    size_t bpos = 0, i = 0;
    for (; i < full_hexads; i += 4) {
        std::array<uint8_t, 4> hexads = {
            _base64_dmap[static_cast<size_t>(base64[i + 0])], // NOLINT
            _base64_dmap[static_cast<size_t>(base64[i + 1])], // NOLINT
            _base64_dmap[static_cast<size_t>(base64[i + 2])], // NOLINT
            _base64_dmap[static_cast<size_t>(base64[i + 3])]  // NOLINT
        };
        output[bpos++] = static_cast<uint8_t>((hexads[0] << 2) | (hexads[1] >> 4));
        output[bpos++] = static_cast<uint8_t>((hexads[1] << 4) | (hexads[2] >> 2));
        output[bpos++] = static_cast<uint8_t>((hexads[2] << 6) | (hexads[3])); // NOLINT
    }
    if (padd_count == 2) {
        std::array<uint8_t, 2> hexads = {
            _base64_dmap[static_cast<size_t>(base64[i + 0])], // NOLINT
            _base64_dmap[static_cast<size_t>(base64[i + 1])], // NOLINT
        };
        output[bpos++] = static_cast<uint8_t>((hexads[0] << 2) | (hexads[1] >> 4));
    }
    else if (padd_count == 1) {
        std::array<uint8_t, 3> hexads = {
            _base64_dmap[static_cast<size_t>(base64[i + 0])], // NOLINT
            _base64_dmap[static_cast<size_t>(base64[i + 1])], // NOLINT
            _base64_dmap[static_cast<size_t>(base64[i + 2])]  // NOLINT
        };
        output[bpos++] = static_cast<uint8_t>((hexads[0] << 2) | (hexads[1] >> 4));
        output[bpos++] = static_cast<uint8_t>((hexads[1] << 4) | (hexads[2] >> 2));
    }

    memcpy(dst, output.get(), size);
}

inline string base64_encode(string_view str) {
    return base64_encode(str.data(), str.size());
}

inline string base64_encode(span<byte> bytes) {
    return base64_encode(bytes.data(), static_cast<size_t>(bytes.size()));
}

inline vector<byte> base64_decode(string_view base64_str) {
    vector<byte> result(base64_byte_size(base64_str));
    base64_decode(result.data(), base64_str);
    return result;
}

} // namespace core

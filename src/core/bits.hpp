#pragma once
#include "types.hpp"

namespace core // NOLINT
{
namespace details
{
#ifdef __GNUC__
    __attribute__((const)) inline uint32_t next_pow2(uint32_t x) {
        return x == 1
                   ? 2
                   : (x > (numlim<uint32_t>::max() / 2 + 1)
                          ? numlim<uint32_t>::max()
                          : (1 << (32U - static_cast<uint32_t>(__builtin_clz(x - 1))))); // NOLINT
    }
    __attribute__((const)) inline uint64_t next_pow2(uint64_t x) {
        return x == 1
                   ? 2
                   : (x > (numlim<uint64_t>::max() / 2 + 1)
                          ? numlim<uint64_t>::max()
                          : (1 << (64U - static_cast<uint64_t>(__builtin_clzll(x - 1))))); // NOLINT
    }
#else
    inline uint32_t next_pow2(uint32_t x) {
        if (x == 1)
            return 2;
        else {
            --x;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            ++x;
            return x;
        }
    }
    inline uint64_t next_pow2(uint64_t x) {
        if (x == 1)
            return 2;
        else {
            --x;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            x |= x >> 16;
            x |= x >> 32;
            ++x;
            return x;
        }
    }

#endif
    inline uint8_t next_pow2(uint8_t x) {
        if (x == 1)
            return 2;
        else {
            --x;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            ++x;
            return x;
        }
    }
    inline uint16_t next_pow2(uint16_t x) {
        if (x == 1)
            return 2;
        else {
            --x;
            x |= x >> 1;
            x |= x >> 2;
            x |= x >> 4;
            x |= x >> 8;
            ++x;
            return x;
        }
    }

} // namespace details

constexpr inline int8_t ilog2(uint64_t value) {
    constexpr int8_t table[64] = {
        63,  0, 58,  1, 59, 47, 53,  2,
        60, 39, 48, 27, 54, 33, 42,  3,
        61, 51, 37, 40, 49, 18, 28, 20,
        55, 30, 34, 11, 43, 14, 22,  4,
        62, 57, 46, 52, 38, 26, 32, 41,
        50, 36, 17, 19, 29, 10, 13, 21,
        56, 45, 25, 31, 35, 16,  9, 12,
        44, 24, 15,  8, 23,  7,  6,  5
    };

    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;

    return table[((value - (value >> 1)) * 0x07EDD5E59A4E28C2) >> 58];
}

constexpr inline int8_t ilog2(uint32_t value) {
    constexpr int8_t table[32] = {
        0,  9,  1,  10, 13, 21, 2,  29,
        11, 14, 16, 18, 22, 25, 3,  30,
        8,  12, 20, 28, 15, 17, 24, 7,
        19, 27, 23, 6,  26, 5,  4,  31
    };

    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    return table[value * 0x07C4ACDD >> 27];
}

constexpr inline int8_t ilog2(uint16_t value) {
    return ilog2(uint32_t(value));
}

constexpr inline int8_t ilog2(uint8_t value) {
    return ilog2(uint32_t(value));
}

template <typename T>
requires std::unsigned_integral<T> T next_power_of_two(T number) {
    return number == 0 ? 0 : details::next_pow2(number);
}

} // namespace core

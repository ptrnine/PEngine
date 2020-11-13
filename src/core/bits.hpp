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

template <typename T>
requires std::unsigned_integral<T> T next_power_of_two(T number) {
    return number == 0 ? 0 : details::next_pow2(number);
}

} // namespace core

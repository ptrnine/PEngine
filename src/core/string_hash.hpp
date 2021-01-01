#pragma once

#include "types.hpp"

namespace core
{
inline u32 hash_djb(span<const byte> data, u32 start = 5381) {
    for (auto b : data) start = (start << 5U) + start + static_cast<u8>(b);
    return start;
}

inline u32 hash_fnv1a32(span<const byte> data, u32 start = 0x811c9dc5) {
    for (auto b : data) start = (start ^ static_cast<u8>(b)) * 0x01000193;
    return start;
}

inline u64 hash_fnv1a64(span<const byte> data, u64 start = 0xcbf29ce484222325) {
    for (auto b : data) start = (start ^ static_cast<u8>(b)) * 0x100000001b3;
    return start;
}

inline u32 hash_djb(string_view str, u32 start = 5381) {
    return hash_djb(
        span(reinterpret_cast<const byte*>(str.data()), static_cast<ptrdiff_t>(str.size())), start);
}

inline u32 hash_fnv1a32(string_view str, u32 start = 0x811c9dc5) {
    return hash_fnv1a32(
        span(reinterpret_cast<const byte*>(str.data()), static_cast<ptrdiff_t>(str.size())), start);
}

inline u64 hash_fnv1a64(string_view str, u64 start = 0xcbf29ce484222325) {
    return hash_fnv1a64(
        span(reinterpret_cast<const byte*>(str.data()), static_cast<ptrdiff_t>(str.size())), start);
}

} // namespace core

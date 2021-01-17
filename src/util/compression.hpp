#pragma once

#include "core/types.hpp"

namespace details {
    template <typename T>
    concept ConstOrNonConstByte = std::same_as<std::remove_const_t<T>, core::byte>;

    template <details::ConstOrNonConstByte T>
    core::vector<core::byte> compress_block(core::span<T> data, int compression_level);
    template <>
    core::vector<core::byte> compress_block(core::span<core::byte> data, int compression_level);
    template <>
    core::vector<core::byte> compress_block(core::span<const core::byte> data, int compression_level);

    template <details::ConstOrNonConstByte T>
    core::vector<core::byte> decompress_block(core::span<T> data, size_t output_size);
    template <>
    core::vector<core::byte> decompress_block(core::span<core::byte> data, size_t output_size);
    template <>
    core::vector<core::byte> decompress_block(core::span<const core::byte> data, size_t output_size);
}

namespace util
{
constexpr int COMPRESS_DEFAULT = -1;
constexpr int COMPRESS_FAST = 1;
constexpr int COMPRESS_BEST = 9;

inline int compress_level_clamp(int level) {
    return std::min(std::max(COMPRESS_DEFAULT, level), COMPRESS_FAST);
}

/**
 * @brief Compress byte input
 *
 * @param data - byte input
 * @param compression_level - -1 - default, 0 - no compression, 1 - best speed, 9 - best compression
 * @param progress - progress callback as void(u64 compressed_count, u64 max_count)
 *
 * @return compressed bytes
 */
core::vector<core::byte> compress(core::span<const core::byte> data,
                                  int                          compression_level = COMPRESS_DEFAULT,
                                  const core::function<void(core::u64, core::u64)>& progress = {});

/**
 * @brief Decompress byte input
 *
 * @param data - byte input
 * @param progress - progress callback as void(u64 decompressed_count, u64 max_count)
 *
 * @return decompressed bytes
 */
core::vector<core::byte>
decompress(core::span<const core::byte>                      data,
           const core::function<void(core::u64, core::u64)>& progress = {});

/**
 * @brief Compress byte input in one iteration
 *
 * @tparam T - core::byte or const core::byte
 * @param compression_level - -1 - default, 0 - no compression, 1 - best speed, 9 - best compression
 * @param byte_data - byte input
 *
 * @return compressed bytes
 */
template <typename T>
core::vector<core::byte> compress_block(T&& byte_data, int compression_level = COMPRESS_DEFAULT) {
    return details::compress_block(
        core::span<std::remove_reference_t<decltype(byte_data[0])>>(byte_data), compression_level);
}

/**
 * @brief Decompress byte input in one iteration
 *
 * @tparam T - core::byte or const core::byte
 * @param byte_data - byte input
 * @param output_size - size of decompressed output
 *
 * @return decompressed bytes
 */
template <typename T>
core::vector<core::byte> decompress_block(T&& byte_data, size_t output_size) {
    return details::decompress_block(
        core::span<std::remove_reference_t<decltype(byte_data[0])>>(byte_data), output_size);
}
} // namespace util

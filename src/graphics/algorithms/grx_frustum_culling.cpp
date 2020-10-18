#include <future>
#include <thread>

#include "grx_frustum_culling.hpp"
#include <core/platform_dependent.hpp>
#include <core/assert.hpp>
#include "grx_frustum_culling_asm.hpp"

constexpr size_t RESERVED_AABBS = 128;

grx::frustum_storage::frustum_storage() {
    aabbs     .reserve(RESERVED_AABBS);
    results   .reserve(RESERVED_AABBS);
    free_aabbs.reserve(RESERVED_AABBS);
}

size_t grx::frustum_storage::new_get_id(const grx_aabb_fast& aabb) {
    if (free_aabbs.empty()) {
        aabbs.emplace_back(aabb);
        results.emplace_back(0xffffffff);
        return aabbs.size() - 1;
    } else {
        auto id = free_aabbs.back();
        free_aabbs.pop_back();
        aabbs.at(id) = aabb;
        return id;
    }
}

size_t grx::frustum_storage::new_get_id() {
    if (free_aabbs.empty()) {
        aabbs.emplace_back();
        results.emplace_back(0xffffffff);
        return aabbs.size() - 1;
    } else {
        auto id = free_aabbs.back();
        free_aabbs.pop_back();
        return id;
    }
}

void grx::frustum_storage::remove_id(size_t id) {
    if (id == std::numeric_limits<size_t>::max())
        return;

    RASSERTF(id < aabbs.size(), "ID >= aabbs size ({}, {})", id, aabbs.size());
    free_aabbs.push_back(id);
}

void grx::frustum_storage::frustum_test_mt(
    std::function<void(void*, void*, void*, std::size_t, uint32_t)> func,
    void*        results,
    void*        aabbs,
    void*        frustum,
    size_t       count,
    frustum_bits bits,
    size_t       nprocs
) {
    std::vector<std::future<void>> futures;

    for (size_t i = 0; i + 1 < nprocs; ++i)
        futures.emplace_back(
            std::async(
                std::launch::async,
                func,
                static_cast<uint32_t*>(results) + i * count,
                static_cast<float*>(aabbs) + i * 8 * count, // NOLINT
                frustum,
                count,
                bits.data()));

    func(static_cast<uint32_t*>(results) + (nprocs - 1) * count,
         static_cast<float*>(aabbs) + (nprocs - 1) * 8 * count, // NOLINT
         frustum,
         count,
         bits.data());
}

void grx::frustum_storage::calculate_culling(const grx_aabb_frustum_planes_fast& frustum, frustum_bits bits) {
    size_t nprocs = std::thread::hardware_concurrency();
    constexpr size_t st_threshold = 12000;

    // AVX
    if (platform_dependent::cpu_ext_check().avx) {
        float frustum_8x_unpacked[6][4][8] __attribute__((aligned(32))); // NOLINT

        for (size_t i = 0; i < 6; ++i) { // NOLINT
            for (size_t j = 0; j < 8; ++j) { // NOLINT
                frustum_8x_unpacked[i][0][j] = frustum.as_array[i].x(); // NOLINT
                frustum_8x_unpacked[i][1][j] = frustum.as_array[i].y(); // NOLINT
                frustum_8x_unpacked[i][2][j] = frustum.as_array[i].z(); // NOLINT
                frustum_8x_unpacked[i][3][j] = frustum.as_array[i].w(); // NOLINT
            }
        }

        auto count_per_thread = aabbs.size() / nprocs;

        if (aabbs.size() > st_threshold && count_per_thread > 8) { // NOLINT
            count_per_thread = count_per_thread - (count_per_thread % 8); // NOLINT must be multiple of 8
            auto remainder_pos = count_per_thread * nprocs;

            frustum_test_mt(x86_64_avx_frustum_culling,
                            results.data(),
                            aabbs.data(),
                            &frustum_8x_unpacked[0][0][0],
                            count_per_thread,
                            bits,
                            nprocs);

            frustum_test(remainder_pos, frustum, bits);
        } else {
            auto count = (aabbs.size() / 8) * 8; // NOLINT

            if (count != 0) {
                frustum_test_mt(x86_64_avx_frustum_culling,
                                results.data(),
                                aabbs.data(),
                                &frustum_8x_unpacked[0][0][0],
                                count,
                                bits,
                                1);
            }

            frustum_test(count, frustum, bits);
        }
    }
    // SSE2
    else if (platform_dependent::cpu_ext_check().sse2) {
        float frustum_4x_unpacked[6][4][4] __attribute__((aligned(32))); // NOLINT

        for (size_t i = 0; i < 6; ++i) { // NOLINT
            for (size_t j = 0; j < 4; ++j) {
                frustum_4x_unpacked[i][0][j] = frustum.as_array[i].x(); // NOLINT
                frustum_4x_unpacked[i][1][j] = frustum.as_array[i].y(); // NOLINT
                frustum_4x_unpacked[i][2][j] = frustum.as_array[i].z(); // NOLINT
                frustum_4x_unpacked[i][3][j] = frustum.as_array[i].w(); // NOLINT
            }
        }

        auto count_per_thread = aabbs.size() / nprocs;

        if (aabbs.size() > st_threshold && count_per_thread > 4) {
            count_per_thread = count_per_thread - (count_per_thread % 4); // must be multiple of 4
            auto remainder_pos = count_per_thread * nprocs;

            frustum_test_mt(x86_64_sse_frustum_culling,
                            results.data(),
                            aabbs.data(),
                            &frustum_4x_unpacked[0][0][0],
                            count_per_thread,
                            bits,
                            nprocs);

            frustum_test(remainder_pos, frustum, bits);
        } else {
            auto count = (aabbs.size() / 4) * 4;

            if (count != 0)
                frustum_test_mt(x86_64_sse_frustum_culling,
                                results.data(),
                                aabbs.data(),
                                &frustum_4x_unpacked[0][0][0],
                                count,
                                bits,
                                1);

            frustum_test(count, frustum, bits);
        }
    }
    else {
        frustum_test(0, frustum, bits);
    }
}


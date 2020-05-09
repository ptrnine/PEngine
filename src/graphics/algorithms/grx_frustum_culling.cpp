#include <thread>
#include "grx_frustum_culling.hpp"

#include <core/platform_dependent.hpp>
#include <core/assert.hpp>
#include "grx_frustum_culling_asm.hpp"

grx::frustum_storage::frustum_storage() {
    aabbs     .reserve(65536);
    results   .reserve(65536);
    free_aabbs.reserve(65536);
}

size_t grx::frustum_storage::new_get_id(const grx_aabb_fast& aabb) {
    if (free_aabbs.empty()) {
        aabbs.emplace_back(aabb);
        results.emplace_back(0);
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
        results.emplace_back(0);
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
    std::function<void(int32_t*, float*, float*, std::size_t)> func,
    int32_t* results,
    float* aabbs,
    float* frustum,
    size_t count,
    size_t nprocs
) {
    std::vector<std::thread> threads;

    for (size_t i = 0; i < nprocs; ++i)
        threads.emplace_back(
            std::thread(
                func,
                results + i * count,
                aabbs + i * 8 * count,
                frustum,
                count));

    for (auto& t : threads)
        t.join();
}

void grx::frustum_storage::calculate_culling(const grx_aabb_frustum_planes_fast& frustum) {
    size_t nprocs = std::thread::hardware_concurrency();
    size_t st_threshold = 512;

    // AVX
    if (platform_dependent::cpu_ext_check().avx) {
        float frustum_8x_unpacked[6][4][8] __attribute__((aligned(32)));

        for (size_t i = 0; i < 6; ++i) {
            for (size_t j = 0; j < 8; ++j) {
                frustum_8x_unpacked[i][0][j] = frustum.as_array[i].x();
                frustum_8x_unpacked[i][1][j] = frustum.as_array[i].y();
                frustum_8x_unpacked[i][2][j] = frustum.as_array[i].z();
                frustum_8x_unpacked[i][3][j] = frustum.as_array[i].w();
            }
        }

        auto count_per_thread = aabbs.size() / nprocs;

        if (aabbs.size() > st_threshold && count_per_thread > 8) {
            count_per_thread = count_per_thread - (count_per_thread % 8); // must be multiple of 8
            auto remainder_pos = count_per_thread * nprocs;

            frustum_test_mt(x86_64_avx_frustum_culling,
                            results.data(),
                            reinterpret_cast<float*>(aabbs.data()),
                            &frustum_8x_unpacked[0][0][0],
                            count_per_thread,
                            nprocs);

            frustum_test(remainder_pos, frustum);
        } else {
            frustum_test(0, frustum);
        }
    }
    // SSE2
    else if (platform_dependent::cpu_ext_check().sse2) {
        float frustum_4x_unpacked[6][4][4] __attribute__((aligned(32)));

        for (size_t i = 0; i < 6; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                frustum_4x_unpacked[i][0][j] = frustum.as_array[i].x();
                frustum_4x_unpacked[i][1][j] = frustum.as_array[i].y();
                frustum_4x_unpacked[i][2][j] = frustum.as_array[i].z();
                frustum_4x_unpacked[i][3][j] = frustum.as_array[i].w();
            }
        }

        auto count_per_thread = aabbs.size() / nprocs;

        if (aabbs.size() > st_threshold && count_per_thread > 4) {
            count_per_thread = count_per_thread - (count_per_thread % 4); // must be multiple of 4
            auto remainder_pos = count_per_thread * nprocs;

            frustum_test_mt(x86_64_sse_frustum_culling,
                            results.data(),
                            reinterpret_cast<float*>(aabbs.data()),
                            &frustum_4x_unpacked[0][0][0],
                            count_per_thread,
                            nprocs);

            frustum_test(remainder_pos, frustum);
        } else {
            frustum_test(0, frustum);
        }
    }
    else {
        frustum_test(0, frustum);
    }
}


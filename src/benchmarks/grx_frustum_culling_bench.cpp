#include <array>
#include <vector>
#include <random>
#include <ctime>
#include <iostream>

#include <iomanip>
#include <thread>
#include <future>

#include <benchmark/benchmark.h>

#include <graphics/algorithms/grx_frustum_culling.hpp>
#include <graphics/algorithms/grx_frustum_culling_asm.hpp>


using namespace grx;
using core::vec;
using core::vec3f;
using core::vec4f;
using core::vector;

int32_t frustum_test(const grx_aabb_fast& aabb, const grx_aabb_frustum_planes_fast& frustum) {
    int32_t pass = 0;
    for (auto& plane : frustum.as_array)
        pass |= (std::max(aabb.min.x() * plane.x(), aabb.max.x() * plane.x()) +
                 std::max(aabb.min.y() * plane.y(), aabb.max.y() * plane.y()) +
                 std::max(aabb.min.z() * plane.z(), aabb.max.z() * plane.z()) + plane.w()) <= 0;
    return pass;
}

grx_aabb_frustum_planes_fast generateFrustum() {
    auto frustum __attribute__((aligned(32))) = grx_aabb_frustum_planes_fast{
            vec{ -0.758738f,    -0.99875f,    0.0684196f,    3.01219f   },
            vec{ 0.761164f,     -0.99875f,    0.0315091f,    -3.71924f  },
            vec{ 0.0339817f,    -0.931207f,   1.3993f,       1.33221f   },
            vec{ -0.0315549f,   -1.06629f,    -1.29937f,     -2.03926f  },
            vec{ 0.00242686f,   -1.9976f,     0.0999336f,    -0.907087f },
            vec{ -1.18627e-07f, 9.76324e-05f, -4.88386e-06f, 0.200044f  }
    };

    return frustum;
}


auto mt = std::mt19937(clock());

auto generateAABBs(
        const vec3f& min,
        const vec3f& max,
        const vec3f& displacementMin,
        const vec3f& displacementMax,
        std::size_t count)
{
    // Distributions for sizes
    auto urd_x = std::uniform_real_distribution<float>(min.x(), max.x());
    auto urd_y = std::uniform_real_distribution<float>(min.y(), max.y());
    auto urd_z = std::uniform_real_distribution<float>(min.z(), max.z());

    // Distributions for positions
    auto urd_d_x = std::uniform_real_distribution<float>(displacementMin.x(), displacementMax.x());
    auto urd_d_y = std::uniform_real_distribution<float>(displacementMin.y(), displacementMax.y());
    auto urd_d_z = std::uniform_real_distribution<float>(displacementMin.z(), displacementMax.z());

    auto res = vector<grx_aabb_fast, core::aligned_allocator<grx_aabb_fast, 32>>();
    res.reserve(count);

    for (std::size_t i = 0; i < count; ++i) {
        auto one = vec{urd_x(mt), urd_y(mt), urd_z(mt)};
        auto two = vec{urd_x(mt), urd_y(mt), urd_z(mt)};

        auto displacement = vec{urd_d_x(mt), urd_d_y(mt), urd_d_z(mt)};
        one += displacement;
        two += displacement;

        // sort values
        if (one.x() > two.x()) std::swap(one.x(), two.x());
        if (one.y() > two.y()) std::swap(one.y(), two.y());
        if (one.z() > two.z()) std::swap(one.z(), two.z());

        res.emplace_back(one, two);
    }

    return res;
}


#define DECL_SIMPLE_FC(COUNT) \
static void BM_simple_frustum_culling_##COUNT##_aabbs(benchmark::State& state) { \
    auto results = vector<int32_t, core::aligned_allocator<int32_t, 32>>(COUNT); \
    auto frustum = generateFrustum(); \
    auto aabbs = generateAABBs(vec3f{0, 0, 0}, vec3f{10, 10, 10}, vec3f{-100, -100, -100}, vec3f{100, 100, 100}, COUNT); \
\
    for (auto _ : state) \
        for (std::size_t i = 0; i < COUNT; ++i) \
            results[i] = frustum_test(aabbs[i], frustum); \
}
DECL_SIMPLE_FC(120)
DECL_SIMPLE_FC(1000)
DECL_SIMPLE_FC(10000)
DECL_SIMPLE_FC(100000)
DECL_SIMPLE_FC(1000000)
DECL_SIMPLE_FC(10000000)


#define DECL_SSE_FC(COUNT) \
static void BM_sse_frustum_culling_##COUNT##_aabbs(benchmark::State& state) { \
    auto res  = vector<int32_t, core::aligned_allocator<int32_t, 32>>(COUNT); \
    auto aabbs = generateAABBs(vec3f{0, 0, 0}, vec3f{10, 10, 10}, vec3f{-100, -100, -100}, vec3f{100, 100, 100}, COUNT); \
    auto frustum = generateFrustum(); \
\
    float frustum_4x_unpacked[6][4][4] __attribute__((aligned(32))); \
    for (size_t i = 0; i < 6; ++i) { \
        for (size_t j = 0; j < 4; ++j) { \
            frustum_4x_unpacked[i][0][j] = frustum.as_array[i].x(); \
            frustum_4x_unpacked[i][1][j] = frustum.as_array[i].y(); \
            frustum_4x_unpacked[i][2][j] = frustum.as_array[i].z(); \
            frustum_4x_unpacked[i][3][j] = frustum.as_array[i].w(); \
        } \
    } \
\
    for (auto _ : state) { \
        x86_64_sse_frustum_culling( \
                res.data(), \
                reinterpret_cast<float *>(aabbs.data()), \
                &frustum_4x_unpacked[0][0][0], \
                COUNT, 1); \
    } \
}
DECL_SSE_FC(120)
DECL_SSE_FC(1000)
DECL_SSE_FC(10000)
DECL_SSE_FC(100000)
DECL_SSE_FC(1000000)
DECL_SSE_FC(10000000)



#define DECL_AVX_FC(COUNT) \
static void BM_avx_frustum_culling_##COUNT##_aabbs(benchmark::State& state) { \
    auto res  = vector<int32_t, core::aligned_allocator<int32_t, 32>>(COUNT); \
    auto aabbs = generateAABBs(vec3f{0, 0, 0}, vec3f{10, 10, 10}, vec3f{-100, -100, -100}, vec3f{100, 100, 100}, COUNT); \
    auto frustum = generateFrustum(); \
\
    float frustum_8x_unpacked[6][4][8] __attribute__((aligned(32))); \
    for (size_t i = 0; i < 6; ++i) { \
        for (size_t j = 0; j < 8; ++j) { \
            frustum_8x_unpacked[i][0][j] = frustum.as_array[i].x(); \
            frustum_8x_unpacked[i][1][j] = frustum.as_array[i].y(); \
            frustum_8x_unpacked[i][2][j] = frustum.as_array[i].z(); \
            frustum_8x_unpacked[i][3][j] = frustum.as_array[i].w(); \
        } \
    } \
\
    for (auto _ : state) { \
        x86_64_avx_frustum_culling( \
                res.data(), \
                reinterpret_cast<float*>(aabbs.data()), \
                &frustum_8x_unpacked[0][0][0], \
                COUNT, 1); \
    } \
}
DECL_AVX_FC(120)
DECL_AVX_FC(1000)
DECL_AVX_FC(10000)
DECL_AVX_FC(100000)
DECL_AVX_FC(1000000)
DECL_AVX_FC(10000000)

/*
void sse_multithread_frustum(int32_t* results, float* aabbs, float* frustum, std::size_t count) {
    std::size_t l0 = 0;
    std::size_t l1 = count >> 1;//2;

    auto f = std::async(std::launch::async, x86_64_sse_frustum_culling, results + l0, aabbs + l0 * 8, frustum, l1);
    x86_64_sse_frustum_culling(results + l1, aabbs + l1 * 8, frustum, l1);
}

static void BM_sse_frustum_culling_mt(benchmark::State& state) {
    auto res  = vector<int32_t, core::aligned_allocator<int32_t, 32>>(COUNT);
    auto res2 = vector<int32_t, core::aligned_allocator<int32_t, 32>>(COUNT);
    auto aabbs = generateAABBs(vec3f{0, 0, 0}, vec3f{10, 10, 10}, vec3f{-100, -100, -100}, vec3f{100, 100, 100}, COUNT);
    auto frustum = generateFrustum();

    float frustum_4x_unpacked[6][4][4] __attribute__((aligned(32)));
    for (size_t i = 0; i < 6; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            frustum_4x_unpacked[i][0][j] = frustum.as_array[i].x();
            frustum_4x_unpacked[i][1][j] = frustum.as_array[i].y();
            frustum_4x_unpacked[i][2][j] = frustum.as_array[i].z();
            frustum_4x_unpacked[i][3][j] = frustum.as_array[i].w();
        }
    }

    for (auto _ : state) {
        sse_multithread_frustum(
                res.data(),
                reinterpret_cast<float *>(aabbs.data()),
                &frustum_4x_unpacked[0][0][0],
                COUNT);
    }


#ifdef TEST_OUTPUT

    for (auto _ : state)
        for (std::size_t i = 0; i < COUNT; ++i)
            res2[i] = frustum_test(aabbs[i], frustum);


    std::cout << "SSE ASM  : ";
    for (auto r : res)
        std::cout << ((r != 0) ? "in " : "out") << " ";
    std::cout << std::endl;

    std::cout << "PLAIN C++: ";
    for (auto r : res2)
        std::cout << ((r != 0) ? "in " : "out") << " ";
    std::cout << std::endl;
    std::cout << std::endl;

#endif

}

void avx_multithread_frustum(int32_t* results, float* aabbs, float* frustum, std::size_t count) {
    std::size_t l0 = 0;
    std::size_t l1 = count >> 1;

    auto f = std::async(std::launch::async, x86_64_avx_frustum_culling, results + l0, aabbs + l0 * 8, frustum, l1);
    x86_64_avx_frustum_culling(results + l1, aabbs + l1 * 8, frustum, l1);
}

static void BM_avx_frustum_culling_mt(benchmark::State& state) {
    auto res  = vector<int32_t, core::aligned_allocator<int32_t, 32>>(COUNT);
    auto res2 = vector<int32_t, core::aligned_allocator<int32_t, 32>>(COUNT);
    auto aabbs = generateAABBs(vec3f{0, 0, 0}, vec3f{10, 10, 10}, vec3f{-100, -100, -100}, vec3f{100, 100, 100}, COUNT);
    auto frustum = generateFrustum();

    float frustum_8x_unpacked[6][4][8] __attribute__((aligned(32)));
    for (size_t i = 0; i < 6; ++i) {
        for (size_t j = 0; j < 8; ++j) {
            frustum_8x_unpacked[i][0][j] = frustum.as_array[i].x();
            frustum_8x_unpacked[i][1][j] = frustum.as_array[i].y();
            frustum_8x_unpacked[i][2][j] = frustum.as_array[i].z();
            frustum_8x_unpacked[i][3][j] = frustum.as_array[i].w();
        }
    }

    for (auto _ : state) {
        avx_multithread_frustum(
                res.data(),
                reinterpret_cast<float *>(aabbs.data()),
                &frustum_8x_unpacked[0][0][0],
                COUNT);
    }

#ifdef TEST_OUTPUT

    for (auto _ : state)
        for (std::size_t i = 0; i < COUNT; ++i)
            res2[i] = frustum_test(aabbs[i], frustum);


    std::cout << "AVX ASM MT:";
    for (auto r : res)
        std::cout << ((r != 0) ? "in " : "out") << " ";
    std::cout << std::endl;

    std::cout << "PLAIN C++: ";
    for (auto r : res2)
        std::cout << ((r != 0) ? "in " : "out") << " ";
    std::cout << std::endl;
    std::cout << std::endl;

#endif

}
*/

#define DECL_PE_FC(COUNT) \
static void BM_PEngine_frustum_culling_##COUNT##_aabbs(benchmark::State& state) { \
    auto aabbs   = generateAABBs(vec3f{0, 0, 0}, vec3f{10, 10, 10}, vec3f{-100, -100, -100}, vec3f{100, 100, 100}, COUNT); \
    auto frustum = generateFrustum(); \
\
    auto proxies = vector<grx_aabb_culling_proxy>(COUNT); \
    for (auto& [proxy, aabb] : core::zip_view(proxies, aabbs)) \
        proxy.aabb() = aabb; \
\
    for (auto _ : state) { \
        grx::grx_frustum_mgr().calculate_culling(frustum); \
    } \
}

DECL_PE_FC(120)
DECL_PE_FC(1000)
DECL_PE_FC(10000)
DECL_PE_FC(100000)
DECL_PE_FC(1000000)
DECL_PE_FC(10000000)

BENCHMARK(BM_simple_frustum_culling_120_aabbs);
BENCHMARK(BM_sse_frustum_culling_120_aabbs);
BENCHMARK(BM_avx_frustum_culling_120_aabbs);
BENCHMARK(BM_PEngine_frustum_culling_120_aabbs);

BENCHMARK(BM_simple_frustum_culling_1000_aabbs);
BENCHMARK(BM_sse_frustum_culling_1000_aabbs);
BENCHMARK(BM_avx_frustum_culling_1000_aabbs);
BENCHMARK(BM_PEngine_frustum_culling_1000_aabbs);

BENCHMARK(BM_simple_frustum_culling_10000_aabbs);
BENCHMARK(BM_sse_frustum_culling_10000_aabbs);
BENCHMARK(BM_avx_frustum_culling_10000_aabbs);
BENCHMARK(BM_PEngine_frustum_culling_10000_aabbs);

BENCHMARK(BM_simple_frustum_culling_100000_aabbs);
BENCHMARK(BM_sse_frustum_culling_100000_aabbs);
BENCHMARK(BM_avx_frustum_culling_100000_aabbs);
BENCHMARK(BM_PEngine_frustum_culling_100000_aabbs);

BENCHMARK(BM_simple_frustum_culling_1000000_aabbs);
BENCHMARK(BM_sse_frustum_culling_1000000_aabbs);
BENCHMARK(BM_avx_frustum_culling_1000000_aabbs);
BENCHMARK(BM_PEngine_frustum_culling_1000000_aabbs);

BENCHMARK(BM_simple_frustum_culling_10000000_aabbs);
BENCHMARK(BM_sse_frustum_culling_10000000_aabbs);
BENCHMARK(BM_avx_frustum_culling_10000000_aabbs);
BENCHMARK(BM_PEngine_frustum_culling_10000000_aabbs);


//BENCHMARK(BM_sse_frustum_culling);
//BENCHMARK(BM_avx_frustum_culling);
//BENCHMARK(BM_sse_frustum_culling_mt);
//BENCHMARK(BM_avx_frustum_culling_mt);

BENCHMARK_MAIN();


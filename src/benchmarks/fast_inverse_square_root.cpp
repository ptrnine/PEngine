#include <benchmark/benchmark.h>
#include <core/vec.hpp>
#include <random>

constexpr size_t VECTOR_DIMM = 100;

template <typename T>
static inline void plain_normalize_tmpl(benchmark::State& state) {
    auto gen = std::bind(std::uniform_real_distribution<T>(-100, 100), std::mt19937()); // NOLINT
    core::array<T, VECTOR_DIMM> a;
    std::generate(a.begin(), a.end(), gen);
    auto v1 = core::vec<T, VECTOR_DIMM>{a};
    volatile T sum = 0;

    for (auto _ : state) {
        sum = sum + v1.normalize().magnitude();
    }
}

template <typename T, size_t Iters>
static inline void fast_normalize_tmpl(benchmark::State& state) {
    auto gen = std::bind(std::uniform_real_distribution<T>(-100, 100), std::mt19937()); // NOLINT
    core::array<T, VECTOR_DIMM> a;
    std::generate(a.begin(), a.end(), gen);
    auto v1 = core::vec<T, VECTOR_DIMM>{a};
    volatile T sum = 0;

    for (auto _ : state) {
        sum = sum + v1.template fast_normalize<Iters>().template fast_magnitude<Iters>();
    }
}

static void plain_normalize_float(benchmark::State& state) {
    plain_normalize_tmpl<float>(state);
}
static void plain_normalize_double(benchmark::State& state) {
    plain_normalize_tmpl<double>(state);
}
static void fast_normalize_1_iters_float(benchmark::State& state) {
    fast_normalize_tmpl<float, 1>(state);
}
static void fast_normalize_1_iters_double(benchmark::State& state) {
    fast_normalize_tmpl<double, 1>(state);
}
static void fast_normalize_2_iters_float(benchmark::State& state) {
    fast_normalize_tmpl<float, 2>(state);
}
static void fast_normalize_2_iters_double(benchmark::State& state) {
    fast_normalize_tmpl<double, 2>(state);
}

BENCHMARK(plain_normalize_float);
BENCHMARK(plain_normalize_double);
BENCHMARK(fast_normalize_1_iters_float);
BENCHMARK(fast_normalize_1_iters_double);
BENCHMARK(fast_normalize_2_iters_float);
BENCHMARK(fast_normalize_2_iters_double);

BENCHMARK_MAIN();


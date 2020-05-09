#include <core/container_extensions.hpp>
#include <benchmark/benchmark.h>
#include <random>

static auto gen = std::bind(std::uniform_int_distribution<size_t>(0, 100) ,std::mt19937());
core::vector<size_t> vector(10000);

static void index_for_default(benchmark::State& state) {
    for (auto _ : state) {
        for (size_t i = 0; i < vector.size(); ++i) {
            vector[i] = gen();
        }
    }
}

static void index_for_core_lib(benchmark::State& state) {
    for (auto _ : state) {
        for (auto i : core::index_view(vector.begin(), vector.end())) {
            vector[i] = gen();
        }
    }
}

static void value_index_for_default(benchmark::State& state) {
    for (auto _ : state) {
        for (size_t i = 0; auto& v : vector) {
            v = gen() * ++i;
        }
    }
}

static void value_index_for_core_lib(benchmark::State& state) {
    for (auto _ : state) {
        for (auto [v, i] : core::value_index_view(vector.begin(), vector.end())) {
            v = gen() * ++i;
        }
    }
}

BENCHMARK(index_for_default);
BENCHMARK(index_for_core_lib);
BENCHMARK(value_index_for_default);
BENCHMARK(value_index_for_core_lib);
BENCHMARK_MAIN();


git_submodule_build(benchmark CMAKE_ARGS -DBENCHMARK_ENABLE_TESTING=OFF)
find_package(benchmark REQUIRED)

add_executable(benchmark_algo    algo.cpp)
add_executable(benchmark_frustum grx_frustum_culling_bench.cpp)

target_link_libraries(benchmark_algo    benchmark::benchmark)
target_link_libraries(benchmark_frustum benchmark::benchmark pe_graphics)

target_include_directories(benchmark_algo    PRIVATE ../)
target_include_directories(benchmark_frustum PRIVATE ../)


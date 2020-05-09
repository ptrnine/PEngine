git_submodule_build(benchmark CMAKE_ARGS -DBENCHMARK_ENABLE_TESTING=OFF)
find_package(benchmark REQUIRED)

add_executable(benchmark_algo algo.cpp)
target_link_libraries(benchmark_algo benchmark::benchmark)
target_include_directories(benchmark_algo PRIVATE ../)


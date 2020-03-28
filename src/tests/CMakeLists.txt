git_submodule_build(Catch2 CMAKE_ARGS -DCATCH_INSTALL_DOCS=OFF -DCATCH_BUILD_TESTING=OFF -DCATCH_INSTALL_HELPERS=ON)
find_package(Catch2 REQUIRED)
enable_testing()

message("flags : ${CMAKE_CXX_FLAGS}")

add_executable(
        tests
        tests.cpp
        print.cpp
        config_manager.cpp
        vec.cpp serialization.cpp)

target_link_libraries(
        tests
        Catch2::Catch2
)

include(CTest)
include(Catch)
catch_discover_tests(tests)

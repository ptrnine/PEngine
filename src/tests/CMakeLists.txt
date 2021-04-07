git_submodule_build(Catch2 CMAKE_ARGS -DCATCH_INSTALL_DOCS=OFF -DCATCH_BUILD_TESTING=OFF -DCATCH_INSTALL_HELPERS=ON)
find_package(Catch2 REQUIRED)
enable_testing()

add_executable(
        tests
        tests.cpp
        print.cpp
        config_manager.cpp
        vec.cpp
        serialization.cpp
        container_extensions.cpp
        try_opt.cpp
        async.cpp
        base64.cpp
        match_syntax.cpp
        md5.cpp
        log.cpp
        grx_cpu_mesh_group.cpp
        grx_skeleton.cpp
        grx_animation.cpp
        compression.cpp
        ranges.cpp
        )

target_link_libraries(
        tests
        Catch2::Catch2
        ${PE_LIBS}
        pe_util
        pe_graphics
)

include(CTest)
include(Catch)
catch_discover_tests(tests)

if (ENABLE_FUZZING_TESTS)
    include(fuzzing/add_afl_fuzz_test.cmake)
    git_submodule_build_tool(AFLplusplus)
    #git_submodule_build_tool(AFLplusplus/gcc_plugin)
    #git_submodule_build_tool(AFLplusplus/libdislocator)
    add_subdirectory(fuzzing)
endif()

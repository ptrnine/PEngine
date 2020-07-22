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
        failure_opt.cpp
        async.cpp
        base64.cpp
        match_syntax.cpp
        md5.cpp
        log.cpp
        )

target_link_libraries(
        tests
        Catch2::Catch2
        Threads::Threads
)

include(CTest)
include(Catch)
catch_discover_tests(tests)

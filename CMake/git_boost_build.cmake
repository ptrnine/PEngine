macro(git_boost_build)
    #execute_process(
    #    COMMAND "boostrap.sh"
    #    RESULT_VARIABLE result
    #    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/submodules/boost")

    #if (result)
    #    message(FATAL_ERROR "Building boost failed")
    #endif()

    if (NOT boost_ALREADY_BUILT)
        if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            set(_boost_toolset "clang-linux")
        elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            set(_boost_toolset "gcc")
        endif()

        if (CMAKE_BUILD_TYPE MATCHES "^(|RELEASE|MINSIZEREL)$")
            set(_boost_debug_symbols "off")
            set(_boost_variant "release")
        else()
            set(_boost_debug_symbols "on")
            set(_boost_variant "debug")
        endif()

        if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
            set(_boost_optimization "off")
        elseif (CMAKE_BUILD_TYPE MATCHES "^(|RELEASE|RELWITHDEBINFO)$")
            set(_boost_optimization "speed")
        elseif (CMAKE_BUILD_TYPE STREQUAL "MINSIZEREL")
            set(_boost_optimization "size")
        endif()

        execute_process(
            COMMAND ./b2 -d+2 -j4 --layout=system --toolset=${_boost_toolset} variant=${_boost_variant} threading=multi link=shared,static optimization=${_boost_optimization} debug-symbols=${_boost_debug_symbols} pch=off --without-python --prefix=${CMAKE_BINARY_DIR}/3rd install
            RESULT_VARIABLE result2
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/submodules/boost"
            )

        if (result2)
            message(FATAL_ERROR "Building boost failed")
        endif()

        set(boost_ALREADY_BUILT ON CACHE STRING "Is submodule already built")
    endif()

    set(BOOST_LIBS boost_context boost_thread boost_fiber dl)

endmacro()

macro(add_afl_fuzz_test _target)
    set(optionArgs ASAN_MODE)
    set(multiValueArgs SOURCES INCLUDE_DIRS LIBS LINK_DIRS)
    cmake_parse_arguments(${_target} "${optionArgs}" "" "${multiValueArgs}" ${ARGN})

    set(AFL_CC "${PE_TOOLS_DIR}/AFLplusplus/afl-clang++")
    set(AFL_GPLUSPLUS "${PE_TOOLS_DIR}/AFLplusplus/afl-g++")
    set(AFL_CLANG "${PE_TOOLS_DIR}/AFLplusplus/afl-clang")
    set(AFL_CLANGPLUSPLUS "${PE_TOOLS_DIR}/AFLplusplus/afl-clang++")

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(AFL_CXX_COMPILER "${AFL_GPLUSPLUS}")
    elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(AFL_CXX_COMPILER "${AFL_CLANGPLUSPLUS}")
    else()
        message(FATAL_ERROR "Can't recognize compiler with id ${CMAKE_CXX_COMPILER_ID}")
    endif()

    set(AFL_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    if (${_target}_ASAN_MODE)
        set(AFL_CXX_FLAGS "${AFL_CXX_FLAGS} -fsanitize=address")
    else()
        set(${_target}_preload "AFL_PRELOAD=${PE_TOOLS_DIR}/AFLplusplus/libdislocator.so")
    endif()

    set(_include_dirs "")
    set(_include_sys_dirs "")
    set(_link_dirs "")
    set(_libs "")
    set(_sources "")
    set(_ld_lib_path "")

    get_property(_incl_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
    foreach (dir ${_incl_dirs})
        if (dir MATCHES "${CMAKE_SOURCE_DIR}/src")
            list(APPEND _include_dirs "-I${dir}")
        else()
            list(APPEND _include_sys_dirs "-isystem")
            list(APPEND _include_sys_dirs "${dir}")
        endif()
    endforeach()
    foreach (dir ${${_target}_INCLUDE_DIRS})
        list(APPEND _include_dirs "-I ${dir}")
    endforeach()

    get_property(_lnk_dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY LINK_DIRECTORIES)
    foreach (dir ${_lnk_dirs})
        list(APPEND _link_dirs "-L${dir}")
        list(APPEND _ld_lib_path "${dir}")
    endforeach()
    foreach (dir ${${_target}_LINK_DIRS})
        list(APPEND _link_dirs "-L${dir}")
        list(APPEND _ld_lib_path "${dir}")
    endforeach()

    get_property(_libs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY LINK_LIBRARIES)
    foreach (lib ${_libs})
        list(APPEND _libs "-l${lib}")
    endforeach()
    foreach (lib ${${_target}_LIBS})
        list(APPEND _libs "-l${lib}")
    endforeach()

    foreach(source ${${_target}_SOURCES})
        list(APPEND _sources "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    endforeach()

    string(REPLACE " " ";" AFL_CXX_FLAGS ${AFL_CXX_FLAGS})

    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${_target}
        COMMAND
                AFL_CXX=${CMAKE_CXX_COMPILER} ${AFL_CXX_COMPILER} -std=c++${CMAKE_CXX_STANDARD}
                ${AFL_CXX_FLAGS} ${_include_dirs} ${_include_sys_dirs} ${_sources} ${_link_dirs} ${_libs} -o ${CMAKE_CURRENT_BINARY_DIR}/${_target}
        DEPENDS ${_sources} ./afl.cpp
    )

    add_custom_target(${_target}_build ALL
        COMMENT "Compiling AFL fuzz test ${_target}..."
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${_target}
    )

    add_custom_command(TARGET ${_target}_build POST_BUILD
        COMMAND
            LD_LIBRARY_PATH=${_ld_lib_path}
            ${CMAKE_CURRENT_BINARY_DIR}/${_target} GEN_RESOURCES
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT           "Generating workdir for ${_target}"
    )

    add_custom_target(afl_${_target}
        #${${_target}_preload}
        LD_LIBRARY_PATH=${_ld_lib_path}
        AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
        AFL_SKIP_CPUFREQ=1
        "${PE_TOOLS_DIR}/AFLplusplus/afl-fuzz"
            -i "${CMAKE_CURRENT_BINARY_DIR}/${_target}_workdir/corpus"
            -o "${CMAKE_CURRENT_BINARY_DIR}/${_target}_workdir/output"
            "${CMAKE_CURRENT_BINARY_DIR}/${_target}"
        COMMENT "Add afl_'${_target} target for fuzzing"
        DEPENDS ${_target}_build
    )

    add_custom_target(afl_${_target}_resume
        #${${_target}_preload}
        LD_LIBRARY_PATH=${_ld_lib_path}
        AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
        AFL_SKIP_CPUFREQ=1
        "${PE_TOOLS_DIR}/AFLplusplus/afl-fuzz"
            -i-
            -o "${CMAKE_CURRENT_BINARY_DIR}/${_target}_workdir/output"
            "${CMAKE_CURRENT_BINARY_DIR}/${_target}"
        COMMENT "Add afl_'${_target} target for fuzzing resuming"
        DEPENDS ${_target}_build
    )
endmacro()

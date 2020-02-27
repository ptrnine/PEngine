cmake_minimum_required(VERSION 3.10)
project(EvoEngine)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(specific_compiler_flags "-Xclang -fconcepts-ts")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(specific_compiler_flags "-fconcepts")
else()
    message(ERROR "Unsupported compiler")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fstrict-aliasing -Wall -pedantic ${specific_compiler_flags}")

set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/3rd")
include_directories("${CMAKE_BINARY_DIR}/3rd/include")

# CMake modules
include(CMake/git_submodule_build.cmake)
include(CMake/git_submodule_update.cmake)

# Update submodules
git_submodule_update()

# Build submodules
git_submodule_build(GSL)
git_submodule_copy_files(flat_hash_map NO_NAME_INCLUDE INCLUDES flat_hash_map.hpp)

add_executable(EvoEngine main.cpp)
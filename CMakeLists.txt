cmake_minimum_required(VERSION 3.15)
project(EvoEngine)

set(CMAKE_CXX_STANDARD 20)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(specific_compiler_flags "-fconcepts")
endif()

set(cxx_flags
    -fstrict-aliasing
    -Wall
    -Wextra
    -Wpedantic
    -Wcast-align
    -Wconversion
    -Wctor-dtor-privacy
    -Wduplicated-branches
    -Wduplicated-cond
    -Wextra-semi
    -Wfloat-equal
    -Wlogical-op
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    -Wsign-conversion
    -Wsign-promo
    -Wno-unused-function -Wno-missing-braces
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${specific_compiler_flags}")
foreach(flag IN LISTS cxx_flags)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
endforeach()


set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/3rd")
include_directories(SYSTEM "${CMAKE_BINARY_DIR}/3rd/include")
link_directories("${CMAKE_BINARY_DIR}/3rd/lib")
include_directories(src)

# GLM
include_directories(SYSTEM submodules/glm)

# CMake modules
include(CMake/git_submodule_build.cmake)
include(CMake/git_submodule_update.cmake)

# Update submodules
git_submodule_update()

# Build submodules
git_submodule_build(GSL CMAKE_ARGS -DGSL_TEST=OFF)
#git_submodule_build(robin-map)
git_submodule_copy_files(flat_hash_map NO_NAME_INCLUDE INCLUDES flat_hash_map.hpp)


# Build type
string(TOUPPER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE)

if (DEFINED CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE MATCHES "^(|DEBUG|RELEASE|RELWITHDEBINFO|MINSIZEREL)$")
    message(FATAL_ERROR "Invalid CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
endif()

if (NOT DEFINED CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE MATCHES "^(|DEBUG|RELWITHDEBINFO)$")
    set(CMAKE_DEBUG_ENABLED ON)
    add_definitions(-D DEBUG)
elseif()
    add_definitions(-D NDEBUG)
endif()


if (CMAKE_BUILD_TYPE STREQUAL "RELEASE" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    #file(COPY ${CMAKE_BINARY_DIR}/compile_commands.json DESTINATION ${CMAKE_SOURCE_DIR})
endif()

find_package(Threads REQUIRED)

add_subdirectory(src)

add_executable(EvoEngine main.cpp)
target_link_libraries(EvoEngine evo_graphics)

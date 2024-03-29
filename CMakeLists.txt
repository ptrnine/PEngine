cmake_minimum_required(VERSION 3.16)
project(PEngine)

set(CMAKE_CXX_STANDARD 20)

option(ENABLE_FUZZING_TESTS "Enable fuzzing tests with AFLplusplus" ON)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(specific_compiler_flags "-fconcepts -fcoroutines -Wduplicated-branches -Wduplicated-cond -Wlogical-op")
endif()

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

set(cxx_flags
    -fstrict-aliasing
    -Wall
    -Wextra
    -Wpedantic
    -Wcast-align
    -Wconversion
    -Wctor-dtor-privacy
    -Wextra-semi
    -Wfloat-equal
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wredundant-decls
    -Wsign-conversion
    -Wsign-promo
    -Werror=unused-result
    -Wno-unused-function
    -Wno-missing-braces
)

if (CMAKE_BUILD_TYPE MATCHES "^(|RELEASE|RELWITHDEBINFO)$")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Ofast")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${specific_compiler_flags}")
foreach(flag IN LISTS cxx_flags)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
endforeach()

option(ENABLE_ASAN "Enable address sanitizer" OFF)
if(ENABLE_ASAN)
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address")
endif()

option(ENABLE_UBSAN "Enable undefined behavior sanitizer" OFF)
if(ENABLE_UBSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
endif()

option(ENABLE_TSAN "Enable thread sanitizer" OFF)
if(ENABLE_TSAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
endif()

option(ENABLE_NATIVE_ARCH "Build for current arch" OFF)
if(ENABLE_NATIVE_ARCH)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

set(CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/3rd")
include_directories(SYSTEM "${CMAKE_BINARY_DIR}/3rd/include")
link_directories("${CMAKE_BINARY_DIR}/3rd/lib")
include_directories(src)


# NASM init
enable_language(ASM_NASM)

set(BUILD_ARCH ${CMAKE_SYSTEM_PROCESSOR})
message("Architecture: ${BUILD_ARCH}")

set(CAN_USE_ASSEMBLER TRUE)

if (UNIX)
    set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
elseif (WIN32)
    string(APPEND CMAKE_ASM_NASM_FLAGS "-dWIN32=1")
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -DWIN64")
endif()


# GLM
include_directories(SYSTEM submodules/glm)

# CMake modules
set(PE_TOOLS_DIR "${CMAKE_SOURCE_DIR}/submodule_tools")
add_subdirectory(CMake)

# Update submodules
git_submodule_update()

# Build submodules
git_submodule_build(GSL CMAKE_ARGS -DGSL_TEST=OFF)
#git_submodule_build(robin-map)
git_submodule_copy_files(libcuckoo NO_NAME_INCLUDE INCLUDES
        libcuckoo/cuckoohash_map.hh
        libcuckoo/cuckoohash_util.hh
        libcuckoo/cuckoohash_config.hh
        libcuckoo/bucket_container.hh)
git_submodule_copy_files(flat_hash_map NO_NAME_INCLUDE INCLUDES flat_hash_map.hpp)
git_submodule_copy_files(readerwriterqueue INCLUDES atomicops.h readerwriterqueue.h)
git_submodule_copy_files(magic_enum EXPLICIT_INCLUDE_DIR ./ INCLUDES include/magic_enum.hpp)

if (CMAKE_BUILD_TYPE STREQUAL "RELEASE" AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    #file(COPY ${CMAKE_BINARY_DIR}/compile_commands.json DESTINATION ${CMAKE_SOURCE_DIR})
endif()

find_package(Threads REQUIRED)

#include(conanbuildinfo.cmake)
#set(CONAN_SYSTEM_INCLUDES ON)
#conan_basic_setup(TARGETS NO_OUTPUT_DIRS)

git_boost_build()

set(PE_LIBS pthread ${BOOST_LIBS})

add_subdirectory(src)

add_executable(PEngine main.cpp)
target_link_libraries(PEngine pe_util pe_graphics pe_ui ${PE_LIBS})

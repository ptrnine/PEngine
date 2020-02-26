cmake_minimum_required(VERSION 3.14)
project(EvoEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstrict-aliasing -Wall -pedantic")

# CMake modules
include(CMake/git_submodule_build.cmake)
include(CMake/git_submodule_update.cmake)

add_executable(EvoEngine main.cpp)
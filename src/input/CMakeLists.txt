git_submodule_build(gainput CMAKE_ARGS -DGAINPUT_SAMPLES=OFF -DGAINPUT_TESTS=OFF)

set(INP_SOURCES
        inp_input_ctx.cpp
        )

set(INP_HEADERS
        inp_input_ctx.hpp
        )

add_library(evo_input SHARED ${INP_SOURCES})

if (CMAKE_BUILD_TYPE STREQUAL "DEBUG")
    target_link_libraries(evo_input gainput-d)
else()
    target_link_libraries(evo_input gainput)
endif()

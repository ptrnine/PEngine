git_submodule_build(glfw)
git_submodule_build(glfx)

set(GRX_SOURCES
        grx_context.cpp
        grx_shader_manager.cpp
        grx_window.cpp
        grx_render_target.cpp
        grx_vbo_tuple.cpp
        )

set(GRX_HEADERS
        grx_context.hpp
        grx_shader_manager.hpp
        grx_window.hpp
        grx_render_target.hpp
        grx_vbo_tuple.hpp
        grx_postprocess_mgr.hpp
)

add_library(evo_graphics SHARED ${GRX_SOURCES})
target_link_libraries(evo_graphics GL GLEW glfx glfw3)

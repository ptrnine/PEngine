git_submodule_build(glfw)
git_submodule_build(glfx)
git_submodule_build(assimp)
git_submodule_copy_files(stb INCLUDES stb_image.h)

find_package(assimp REQUIRED)

set(GRX_SOURCES
        grx_utils.cpp
        grx_context.cpp
        grx_shader_mgr.cpp
        grx_window.cpp
        grx_render_target.cpp
        grx_vbo_tuple.cpp
        grx_texture_mgr.cpp
        grx_mesh_mgr.cpp
        grx_camera.cpp
        grx_camera_manipulator.cpp
        grx_camera_manipulator_fly.cpp
)

set(GRX_HEADERS
        grx_utils.hpp
        grx_context.hpp
        grx_shader_mgr.hpp
        grx_window.hpp
        grx_render_target.hpp
        grx_vbo_tuple.hpp
        grx_postprocess_mgr.hpp
        grx_texture_mgr.hpp
        grx_mesh_mgr.hpp
        grx_camera.hpp
        grx_camera_manipulator.hpp
        grx_camera_manipulator_fly.hpp
)

add_library(evo_graphics SHARED ${GRX_SOURCES})
target_link_libraries(evo_graphics GL GLEW glfx glfw3 assimp::assimp evo_input)

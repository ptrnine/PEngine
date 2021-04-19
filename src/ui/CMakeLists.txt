git_submodule_copy_files(Nuklear NO_NAME_INCLUDE INCLUDES nuklear.h)
#git_submodule_copy_files(Nuklear
#    EXPLICIT_INCLUDE_DIR ./
#    INCLUDES demo/glfw_opengl3/nuklear_glfw_gl3.h)

set(UI_SOURCES
    nuklear.cpp
    )

set(UI_HEADERS'
    nuklear.hpp
    )

add_library(pe_ui SHARED ${UI_SOURCES})
target_link_libraries(pe_ui ${PE_LIBS})

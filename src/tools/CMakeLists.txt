add_executable(merge_dae_anim merge_dae_anim.cpp)
target_link_libraries(merge_dae_anim ${PE_LIBS})

add_executable(macro_gen macro_gen.cpp)
target_link_libraries(macro_gen ${PE_LIBS})

add_executable(lang_tool lang_tool.cpp)
target_link_libraries(lang_tool ${PE_LIBS})

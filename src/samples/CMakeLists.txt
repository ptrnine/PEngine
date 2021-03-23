add_executable(textured_quad textured_quad.cpp)
target_link_libraries(textured_quad pe_graphics pe_util pe_input ${PE_LIBS})

add_executable(genshit genshit.cpp)
target_link_libraries(genshit pe_graphics pe_util)

add_executable(genshit_gpu genshit_gpu.cpp)
target_link_libraries(genshit_gpu pe_graphics pe_util ${PE_LIBS})

#add_executable(vbo_sample vbo_sample.cpp)
#target_link_libraries(vbo_sample pe_graphics pe_util)

add_executable(fiber_test fiber_test.cpp)
target_link_libraries(fiber_test ${PE_LIBS})

add_executable(input_test input_test.cpp)
target_link_libraries(input_test pe_input X11 glfw3 ${PE_LIBS})

add_executable(glock glock.cpp)
target_link_libraries(glock pe_graphics pe_util pe_input ${PE_LIBS})

add_executable(glock_hud_test glock_hud_test.cpp)
target_link_libraries(glock_hud_test pe_graphics pe_util pe_input ${PE_LIBS})

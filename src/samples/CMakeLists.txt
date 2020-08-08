add_executable(textured_quad textured_quad.cpp)
target_link_libraries(textured_quad pe_graphics pe_input)

add_executable(genshit genshit.cpp)
target_link_libraries(genshit pe_graphics)

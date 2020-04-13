#include "grx_vbo_tuple.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

void grx::_grx_gen_vao_and_vbos(uint* vao, uint* vbos_ptr, size_t vbos_size) {
    glGenVertexArrays(1, vao);
    glBindVertexArray(*vao);
    glGenBuffers(static_cast<GLsizei>(vbos_size), vbos_ptr);
}

void grx::_grx_delete_vao_and_vbos(uint* vao, uint* vbos_ptr, size_t vbos_size) {
    glDeleteBuffers(static_cast<GLsizei>(vbos_size), vbos_ptr);
    glDeleteVertexArrays(1, vao);
}

void grx::_grx_bind_vao(uint vao_id) {
    glBindVertexArray(vao_id);
}

void grx::_grx_bind_vbo(uint gl_target, uint vbo_id) {
    glBindBuffer(gl_target, vbo_id);
}

void grx::_grx_setup_matrix_vbo(uint vbo_id, uint location) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    for (unsigned i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(location + i);
        glVertexAttribPointer(location + i, 4, GL_FLOAT, GL_FALSE,
                              sizeof(glm::mat4), reinterpret_cast<GLvoid*>(sizeof(GLfloat) * i * 4));
        glVertexAttribDivisor(location + i, 1);
    }
}


void grx::_grx_set_data_vector_vec2f_vbo(uint vbo_id, uint location, const vbo_vector_vec2f& data) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vbo_vector_vec2f::value_type) * data.size()),
                 data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
}

void grx::_grx_set_data_vector_vec3f_vbo(uint vbo_id, uint location, const vbo_vector_vec3f& data) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vbo_vector_vec3f::value_type) * data.size()),
                 data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
}

void grx::_grx_set_data_vector_indices_vbo(uint vbo_id, const vbo_vector_indices& data) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vbo_vector_indices::value_type) * data.size()),
                 data.data(), GL_STATIC_DRAW);
}

void grx::_grx_set_data_array_ids_vbo(uint vbo_id, uint location, vbo_array_ids<1>::value_type* data, size_t size) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vbo_array_ids<1>::value_type) * size),
                 data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(location);
    glVertexAttribIPointer(location, static_cast<GLint>(size), GL_INT, 0, nullptr);
}

void grx::_grx_set_data_array_weights_vbo(uint vbo_id, uint location, vbo_array_weights<1>::value_type* data, size_t size) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vbo_array_weights<1>::value_type) * size),
                 data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, static_cast<GLint>(size), GL_FLOAT, GL_FALSE, 0, nullptr);
}

void grx::_grx_draw_elements_base_vertex(size_t indices_count, size_t start_indices_pos, size_t start_vertex_pos) {
    glDrawElementsBaseVertex(
            GL_TRIANGLES,
            static_cast<GLsizei>(indices_count),
            GL_UNSIGNED_INT,
            reinterpret_cast<GLvoid*>(sizeof(uint) * start_indices_pos),
            static_cast<GLint>(start_vertex_pos));
}

void grx::_grx_draw_arrays(size_t vertex_count, size_t start_vertex_pos) {
    glDrawArrays(GL_TRIANGLES, static_cast<GLint>(start_vertex_pos), static_cast<GLsizei>(vertex_count));
}

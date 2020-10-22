#include "grx_vbo_tuple.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "grx_gl_trace.hpp"

namespace grx {

void _grx_gen_vao_and_vbos(win_vao_map_t& vao_map, uint* vbos_ptr, size_t vbos_size) {
    auto current_window = glfwGetCurrentContext();
    PeRelRequireF(current_window, "{}", "Attempt to create grx_vbo_tuple without OpenGL context");

    uint vao; // NOLINT
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(static_cast<GLsizei>(vbos_size), vbos_ptr);

    vao_map.emplace(current_window, vao);
}

void _grx_delete_vao_and_vbos(win_vao_map_t& vao_map, uint* vbos_ptr, size_t vbos_size) {
    glDeleteBuffers(static_cast<GLsizei>(vbos_size), vbos_ptr);

    for (auto [_, vao] : vao_map)
        glDeleteVertexArrays(1, &vao);
}

bool _grx_bind_vao(win_vao_map_t& vao_map) {
    auto current_window = glfwGetCurrentContext();
    PeRelRequireF(current_window, "{}", "Attempt to bind VAO without OpenGL context (!?)");

    auto [position, was_inserted] = vao_map.emplace(current_window, -1);

    if (was_inserted) {
        uint vao; // NOLINT
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        position->second = vao;

        return true;
    }
    else {
        glBindVertexArray(position->second);
    }

    return false;
}

void _grx_bind_vbo(uint gl_target, uint vbo_id) {
    glBindBuffer(gl_target, vbo_id);
}

void _grx_setup_matrix_vbo(uint vbo_id, uint location) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    for (unsigned i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(location + i);
        glVertexAttribPointer(location + i, 4, GL_FLOAT, GL_FALSE,
                              sizeof(glm::mat4), reinterpret_cast<GLvoid*>(sizeof(GLfloat) * i * 4)); // NOLINT
        glVertexAttribDivisor(location + i, 1);
    }
}


void _grx_rebind_vector_vec2f_vbo(uint vbo_id, uint location) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
}

void _grx_set_data_vector_vec2f_vbo(uint vbo_id, uint location, const vbo_vector_vec2f& data) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vbo_vector_vec2f::value_type) * data.size()),
                 data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
}

template <typename T>
core::vector<T> _get_vector_t(uint vbo_id, uint size) {
    core::vector<T> result(size);
    GL_TRACE(glGetNamedBufferSubData,
            vbo_id,
            0,
            static_cast<GLsizeiptr>(size * sizeof(T)),
            result.data()
            );
    return result;
}

vbo_vector_vec2f _get_vector_vec2f(uint vbo_id, uint size) {
    return _get_vector_t<vec2f>(vbo_id, size);
}

vbo_vector_vec3f _get_vector_vec3f(uint vbo_id, uint size) {
    return _get_vector_t<vec3f>(vbo_id, size);
}

vbo_vector_indices _get_vector_indices(uint vbo_id, uint size) {
    return _get_vector_t<vbo_vector_indices::value_type>(vbo_id, size);
}

vbo_vector_bone _get_vector_bone(uint vbo_id, uint size) {
    return _get_vector_t<vbo_vector_bone::value_type>(vbo_id, size);
}

vbo_vector_matrix4 _get_vector_matrix4(uint vbo_id, uint size) {
    return _get_vector_t<vbo_vector_matrix4::value_type>(vbo_id, size);
}


void _grx_rebind_vector_vec3f_vbo(uint vbo_id, uint location) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
}

void _grx_set_data_vector_vec3f_vbo(uint vbo_id, uint location, const vbo_vector_vec3f& data) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vbo_vector_vec3f::value_type) * data.size()),
                 data.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
}


void _grx_rebind_vector_indices_ebo(uint vbo_id) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_id);
}

void _grx_set_data_vector_indices_ebo(uint vbo_id, const vbo_vector_indices& data) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(sizeof(vbo_vector_indices::value_type) * data.size()),
                 data.data(), GL_STATIC_DRAW);
}

void _grx_set_data_vector_matrix_vbo(uint vbo_id, const glm::mat4* matrices, size_t size) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER, GLsizeiptr(sizeof(glm::mat4) * size), matrices, GL_STATIC_DRAW);
}


void _grx_rebind_vector_bone_vbo(uint vbo_id, uint location, size_t bone_per_vertex) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);

    glEnableVertexAttribArray(location);
    glVertexAttribIPointer(
            location,
            static_cast<GLint>(bone_per_vertex),
            GL_INT,
            static_cast<GLsizei>((sizeof(uint) + sizeof(float)) * bone_per_vertex),
            reinterpret_cast<GLvoid*>(0)); // NOLINT

    glEnableVertexAttribArray(location + 1);
    glVertexAttribPointer(
            location + 1,
            static_cast<GLint>(bone_per_vertex),
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>((sizeof(uint) + sizeof(float)) * bone_per_vertex),
            reinterpret_cast<GLvoid*>(sizeof(uint) * bone_per_vertex)); // NOLINT
}

void _grx_set_data_vector_bone_vbo(uint vbo_id, uint location, const void* data, size_t bone_per_vertex, size_t size) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id);
    glBufferData(GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>((sizeof(uint) + sizeof(float)) * bone_per_vertex * size),
            data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(location);
    glVertexAttribIPointer(
            location,
            static_cast<GLint>(bone_per_vertex),
            GL_INT,
            static_cast<GLsizei>((sizeof(uint) + sizeof(float)) * bone_per_vertex),
            reinterpret_cast<GLvoid*>(0)); // NOLINT

    glEnableVertexAttribArray(location + 1);
    glVertexAttribPointer(
            location + 1,
            static_cast<GLint>(bone_per_vertex),
            GL_FLOAT,
            GL_FALSE,
            static_cast<GLsizei>((sizeof(uint) + sizeof(float)) * bone_per_vertex),
            reinterpret_cast<GLvoid*>(sizeof(uint) * bone_per_vertex)); // NOLINT
}

void _grx_draw_elements_base_vertex(size_t indices_count, size_t start_indices_pos, size_t start_vertex_pos) {
    glDrawElementsBaseVertex(
            GL_TRIANGLES,
            static_cast<GLsizei>(indices_count),
            GL_UNSIGNED_INT,
            reinterpret_cast<GLvoid*>(sizeof(uint) * start_indices_pos), // NOLINT
            static_cast<GLint>(start_vertex_pos));
}

void _grx_draw_elements_instanced_base_vertex(
        size_t instances_count,
        size_t indices_count,
        size_t start_indices_pos,
        size_t start_vertex_pos
) {
    glDrawElementsInstancedBaseVertex(
            GL_TRIANGLES,
            static_cast<GLsizei>(indices_count),
            GL_UNSIGNED_INT,
            reinterpret_cast<GLvoid*>(sizeof(uint) * start_indices_pos), // NOLINT
            static_cast<GLsizei>(instances_count),
            static_cast<GLint>(start_vertex_pos));
}

void _grx_draw_arrays(size_t vertex_count, size_t start_vertex_pos) {
    glDrawArrays(GL_TRIANGLES, static_cast<GLint>(start_vertex_pos), static_cast<GLsizei>(vertex_count));
}
} // namespace grx


#include "grx_ssbo.hpp"
#include "grx_gl_trace.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace grx::ssbo_details {

void ssbo_gen(uint* ssbo_name) {
    GL_TRACE(glGenBuffers, 1, ssbo_name);
}

void ssbo_delete(uint ssbo_name) {
    GL_TRACE(glDeleteBuffers, 1, &ssbo_name);
}

void ssbo_bind(uint ssbo_name) {
    GL_TRACE(glBindBuffer, GL_SHADER_STORAGE_BUFFER, ssbo_name);
}

void ssbo_data(uint ssbo_name, uint size, const void* data) {
    glNamedBufferData(ssbo_name, static_cast<GLsizeiptr>(size), data, GL_STATIC_DRAW);
}

void ssbo_setup(uint ssbo_name, uint index, uint size, const void* data) {
    GL_TRACE(glBindBufferBase, GL_SHADER_STORAGE_BUFFER, index, ssbo_name);
    ssbo_data(ssbo_name, size, data);
}

void ssbo_retrieve(uint ssbo_name, uint size, void* data) {
    void* mapped = GL_TRACE(
            glMapNamedBufferRange,
            ssbo_name,
            0,
            static_cast<GLsizeiptr>(size),
            GL_MAP_READ_BIT);

    if (mapped)
        memcpy(data, mapped, size);
    else
        throw std::runtime_error("Can't retrieve data from SSBO");

    if (!GL_TRACE(glUnmapNamedBuffer, ssbo_name))
        throw std::runtime_error("SSBO corruption");
}

} // grx::ssbo_details

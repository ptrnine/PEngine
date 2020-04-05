#include "src/core/config_manager.hpp"
#include "src/core/time.hpp"
#include "src/graphics/grx_context.hpp"
#include "graphics/grx_shader_manager.hpp"
#include "src/graphics/grx_render_target.hpp"
#include "src/graphics/grx_window.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

int main() {
    std::ios_base::sync_with_stdio(false);

    core::config_manager cm;
    grx::grx_shader_manager m;

    auto wnd = grx::grx_window("wnd", {800, 600}, m, cm);
    wnd.make_current();

    auto prg = m.compile_program(cm, "shader_dummy");

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    static const GLfloat g_vertex_buffer_data[] = {
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            0.0f,  1.0f, 0.0f,
    };

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

    core::timer tm;

    while (!wnd.should_close()) {
        wnd.bind_renderer();
        m.use_program(prg);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glDisableVertexAttribArray(0);

        wnd.present();
        wnd.poll_events();

        std::cout << tm.measure<core::milliseconds>() << std::endl;
    }

    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //glDrawArrays(GL_TRIANGLES, 0, 3);

    glDisableVertexAttribArray(0);

    return 0;
}

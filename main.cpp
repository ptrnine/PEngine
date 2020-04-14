#include "src/core/config_manager.hpp"
#include "src/core/time.hpp"
//#include "src/graphics/grx_context.hpp"
#include "graphics/grx_shader_mgr.hpp"
//#include "src/graphics/grx_render_target_tuple.hpp"
#include "src/graphics/grx_window.hpp"
#include "graphics/grx_vbo_tuple.hpp"
#include <graphics/grx_postprocess_mgr.hpp>
#include <graphics/grx_texture_mgr.hpp>

//#include <GL/glew.h>
//#include <GLFW/glfw3.h>

int main() {
    std::ios_base::sync_with_stdio(false);

    core::config_manager cm;
    grx::grx_shader_mgr m;

    auto wnd = grx::grx_window("wnd", {1600, 900}, m, cm);
    wnd.make_current();

    grx::grx_texture_mgr ttm;
    auto t = ttm.load(cm, "cake.jpg");

    wnd.push_postprocess({ m, cm, "shader_vhs2_texture", { "time" }, grx::postprocess_uniform_seconds()});
    wnd.push_postprocess({ m, cm, "shader_vhs1_texture", { "time" }, grx::postprocess_uniform_seconds()});

    auto prg = m.compile_program(cm, "shader_dummy");

    grx::grx_vbo_tuple<grx::vbo_vector_vec3f> vbo_tuple;
    vbo_tuple.set_data<0>({
        {-1.0f, -1.0f, 0.0f },
        { 1.0f, -1.0f, 0.0f },
        { 0.0f,  1.0f, 0.0f },
    });
    core::timer tm;

    while (!wnd.should_close()) {
        wnd.bind_and_clear_render_target();
        m.use_program(prg);

        vbo_tuple.bind_vao();
        vbo_tuple.draw(3);

        wnd.present();
        wnd.poll_events();

        //std::cout << tm.measure<core::milliseconds>() << std::endl;
    }

    return 0;
}

#include "src/core/config_manager.hpp"
#include "src/core/time.hpp"
//#include "src/graphics/grx_context.hpp"
#include "graphics/grx_shader_mgr.hpp"
//#include "src/graphics/grx_render_target_tuple.hpp"
#include "src/graphics/grx_window.hpp"
#include "graphics/grx_vbo_tuple.hpp"
#include <graphics/grx_postprocess_mgr.hpp>
#include <graphics/grx_texture_mgr.hpp>
#include <graphics/grx_mesh_mgr.hpp>
#include <graphics/grx_camera.hpp>
#include <graphics/grx_camera_manipulator_fly.hpp>

int main() {
    std::ios_base::sync_with_stdio(false);

    core::config_manager cm;
    grx::grx_shader_mgr m;

    auto wnd = grx::grx_window("wnd", {1600, 900}, m, cm);
    wnd.make_current();

    auto cam = grx::grx_camera::make_shared({0.f, 0.f, 10.f}, 16.f/9.f);
    cam->create_camera_manipulator<grx::grx_camera_manipulator_fly>(wnd);
    wnd.attach_camera(cam);

    grx::grx_mesh_mgr mm;
    auto mesh = mm.load(cm, "cz805.dae");

    //wnd.push_postprocess({ m, cm, "shader_vhs2_texture", { "time" }, grx::postprocess_uniform_seconds()});
    //wnd.push_postprocess({ m, cm, "shader_vhs1_texture", { "time" }, grx::postprocess_uniform_seconds()});

    auto prg = m.compile_program(cm, "shader_tech_skeleton_textured");
    core::timer tm;

    mesh->translate({0, 0, -20});
    mesh->rotate({glm::half_pi<float>(), 0, glm::pi<float>()});
    //mesh->set_instance_count(100);
    //for (size_t i = 0; i < 100; ++i)
    //    mesh->translate(core::vec3f{i, 0.f, 0.f}, i);

    while (!wnd.should_close()) {
        wnd.bind_and_clear_render_target();
        m.use_program(prg);

        mesh->draw(cam, prg);

        wnd.present();
        wnd.update_input();
    }

    return 0;
}
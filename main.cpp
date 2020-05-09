#include "src/core/config_manager.hpp"
#include "src/core/time.hpp"
//#include "src/graphics/grx_context.hpp"
#include "graphics/grx_shader_mgr.hpp"
//#include "src/graphics/grx_render_target_tuple.hpp"
#include "src/graphics/grx_window.hpp"
#include "graphics/grx_vbo_tuple.hpp"
#include <graphics/grx_postprocess_mgr.hpp>
#include <graphics/grx_texture_mgr.hpp>
#include <graphics/grx_mesh_instance.hpp>
#include <graphics/grx_camera.hpp>
#include <graphics/grx_camera_manipulator_fly.hpp>
#include <graphics/grx_mesh_instance.hpp>
#include <graphics/grx_debug.hpp>
#include <graphics/algorithms/grx_frustum_culling.hpp>
#include <core/fps_counter.hpp>
#include <graphics/grx_color_map.hpp>
#include <core/base64.hpp>

int main() {
    std::ios_base::sync_with_stdio(false);

    core::config_manager cm;
    grx::grx_shader_mgr m;

    //auto wnd2 = grx::grx_window("wnd22", {800, 600}, m, cm);
    auto wnd = grx::grx_window("wnd", {1600, 900}, m, cm);
    wnd.set_pos({300, 0});
    //wnd2.set_pos({300, 0});
    wnd.make_current();


    auto input_map = wnd.create_input_map();
    if (auto map = input_map.lock()) {
        auto keyboard = wnd.keyboard_id();
        map->MapBool('R', keyboard, gainput::KeyR);
    }

    auto cam = grx::grx_camera::make_shared({0.f, 0.f, 10.f}, 16.f/9.f);
    cam->create_camera_manipulator<grx::grx_camera_manipulator_fly>();
    wnd.attach_camera(cam);

    //auto cam2 = grx::grx_camera::make_shared({0.f, 0.f, 10.f}, 8.f/6.f);
    //cam2->create_camera_manipulator<grx::grx_camera_manipulator_fly>();
    //wnd2.attach_camera(cam2);
    //grx::grx_aabb_debug_drawer aabb_debug(cm);

    grx::grx_mesh_mgr mm;

    core::vector<grx::grx_mesh_instance> models;//(100, grx::grx_mesh_instance(cm, mm, "cz805.dae"));
    models.emplace_back(grx::grx_mesh_instance(cm, mm, "cz805.dae"));
    //auto models = grx::grx_mesh_pack(cm, mm, "cz805.dae", 100);
    for (float pos = 0; auto& m : models)
        m.move({pos += 20.f, 20.f, -20.f});

    models.front().set_rotation({-90, 180, 30});
    models.front().set_debug_bone_aabb_draw(true);
    models.front().set_debug_aabb_draw(true);

    //auto mesh = mm.load(cm, "cz805.dae", true);

    //wnd.push_postprocess({ m, cm, "shader_vhs2_texture", { "time" }, grx::postprocess_uniform_seconds()});
    //wnd.push_postprocess({ m, cm, "shader_vhs1_texture", { "time" }, grx::postprocess_uniform_seconds()});

    wnd.push_postprocess({m, cm, "shader_gamma_correction", grx::uniform_pair{"gamma", 1.3f}});

    auto tech = m.load_render_tech(cm, "shader_tech_textured");

    //mesh->translate({0, 0, -20});
    //mesh->rotate({glm::half_pi<float>(), 0, glm::pi<float>()});

    core::update_smoother us(10, 0.8);
    core::fps_counter counter;
    core::timer timer;

    //std::vector<glm::mat4> mmm(1);
    //for (size_t i = 0; i < mmm.size(); ++i)
    //    mmm[i] = glm::translate(glm::mat4(1.f), glm::vec3(static_cast<float>(i), 0.f, 0.f));


    //models[0].play_animation("", false);

    while (!wnd.should_close()) {
        counter.update();
        inp::inp_ctx().update();

        if (auto map = input_map.lock()) {
            if (map->GetBoolIsNew('R'))
                models[0].play_animation("", true);
        }

        if (timer.measure() > 1.f) {
            core::printline("FPS: {}", counter.get());
            timer.reset();
        }

        grx::grx_frustum_mgr().calculate_culling(cam->extract_frustum());

        wnd.make_current();
        wnd.bind_and_clear_render_target();
        //models.draw(cam->view_projection(), tech);
        for (auto& m : models)
            m.draw(cam->view_projection(), tech);

        //grx::grx_aabb_debug().draw(models.front().aabb(), models.front().model_matrix(), cam->view_projection(), {0.f, 1.f, 0.f});

        //mesh->draw_instanced(cam->view_projection(), mmm, tech);
        wnd.present();

        //wnd2.make_current();
        //wnd2.bind_and_clear_render_target();
        //for (auto& m : models)
        //    m.draw(cam2->view_projection(), tech);
        //mesh->draw_instanced(cam2->view_projection(), mmm, tech);
        //models.draw(cam2->view_projection(), tech);
        //wnd2.present();

        wnd.update_input();
        //wnd2.update_input();


        us.smooth();
    }

    return 0;
}

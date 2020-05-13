#include "src/core/config_manager.hpp"
#include "src/core/time.hpp"
//#include "src/graphics/grx_context.hpp"
#include "graphics/grx_shader_mgr.hpp"
//#include "src/graphics/grx_render_target_tuple.hpp"
#include "src/graphics/grx_window.hpp"
#include "graphics/grx_vbo_tuple.hpp"
#include <graphics/grx_postprocess_mgr.hpp>
#include <graphics/grx_mesh_instance.hpp>
#include <graphics/grx_camera.hpp>
#include <graphics/grx_camera_manipulator_fly.hpp>
#include <graphics/grx_mesh_instance.hpp>
#include <graphics/grx_debug.hpp>
#include <graphics/algorithms/grx_frustum_culling.hpp>
#include <core/fps_counter.hpp>
#include <graphics/grx_color_map.hpp>
#include <graphics/grx_texture.hpp>
#include <graphics/grx_texture_mgr.hpp>

#include <core/async.hpp>

int main() {
    std::ios_base::sync_with_stdio(false);

    core::config_manager cm;
    grx::grx_shader_mgr m;

    auto wnd = grx::grx_window("wnd", {1600, 900}, m, cm);
    wnd.set_pos({300, 0});
    wnd.make_current();

    auto input_map = wnd.create_input_map();
    if (auto map = input_map.lock()) {
        auto keyboard = wnd.keyboard_id();
        map->MapBool('R', keyboard, gainput::KeyR);
    }

    auto cam = grx::grx_camera::make_shared({0.f, 0.f, 5.f}, 16.f/9.f);
    cam->create_camera_manipulator<grx::grx_camera_manipulator_fly>();
    wnd.attach_camera(cam);

    auto tmgr = grx::grx_texture_mgr::create_shared(cm);
    //auto tex1 = tmgr->load_unwrap<grx::color_rgb>("/home/ptrnine/Рабочий стол/22.jpg");
    //auto tex2 = tmgr->load_unwrap<grx::color_rgba>("/home/ptrnine/Рабочий стол/22.jpg");
    //auto tex3 = tmgr->load_unwrap<grx::color_rgb>("/home/ptrnine/Рабочий стол/22.jpg");
    //auto tex4 = tmgr->load_unwrap<grx::color_rgba>("/home/ptrnine/Рабочий стол/unnamed.png");
    //std::vector<grx::grx_texture<3>> textures;

    //tex2.set_load_significance(grx::texture_load_significance::low);
    //for (size_t i = 0; i < 1; ++i) {
    //    textures.emplace_back(grx::load_texture_unwrap("/home/ptrnine/Рабочий стол/22.jpg"));
    //}

    grx::grx_vbo_tuple<
        grx::vbo_vector_indices,
        grx::vbo_vector_vec3f,
        grx::vbo_vector_vec2f
    > vbo;
    vbo.set_data<0>({0, 3, 1, 2, 1, 3});
    vbo.set_data<1>({
        {-1.f, -1.f, 0.f},
        {-1.f,  1.f, 0.f},
        { 1.f,  1.f, 0.f},
        { 1.f, -1.f, 0.f}
    });
    vbo.set_data<2>({
        {0.f, 1.f},
        {0.f, 0.f},
        {1.f, 0.f},
        {1.f, 1.f}
    });

    auto t = m.load_render_tech(cm, "shader_tech_textured");

    //grx::grx_mesh_mgr mm;

    //core::vector<grx::grx_mesh_instance> models;
    //models.emplace_back(grx::grx_mesh_instance(cm, mm, "cz805.dae"));
    //for (float pos = 0; auto& m : models)
    //    m.move({pos += 20.f, 20.f, -20.f});

    //models.front().set_rotation({-90, 180, 30});
    //models.front().set_debug_bone_aabb_draw(true);
    //models.front().set_debug_aabb_draw(true);

    //wnd.push_postprocess({ m, cm, "shader_vhs2_texture", { "time" }, grx::postprocess_uniform_seconds()});
    //wnd.push_postprocess({ m, cm, "shader_vhs1_texture", { "time" }, grx::postprocess_uniform_seconds()});
    wnd.push_postprocess({m, cm, "shader_gamma_correction", grx::uniform_pair{"gamma", 1/1.3f}});

    //auto tech = m.load_render_tech(cm, "shader_tech_textured");

    core::update_smoother us(10, 0.8);
    core::fps_counter counter;
    core::timer timer;

    //grx::grx_texture<3> tx({4000, 2300});

    auto tex = tmgr->load_async_unwrap<grx::color_rgb>("/home/ptrnine/Рабочий стол/22.tga");
    //core::optional<grx::grx_texture_id<3>> texopt;

    while (!wnd.should_close()) {
        //if (texture2.is_ready()) {
        //    tx = texture2.get();
        //    core::printline("size: {}", tx.size());
        //}
        //auto texture2 = texture;

        counter.update();
        inp::inp_ctx().update();

        if (auto map = input_map.lock()) {
            //if (map->GetBoolIsNew('R'))
            //    textures.clear();
                //models[0].play_animation("", true);
        }

        if (timer.measure() > core::seconds(1)) {
            core::printline("FPS: {}", counter.get());
            timer.reset();
        }

        grx::grx_frustum_mgr().calculate_culling(cam->extract_frustum());

        wnd.make_current();
        wnd.bind_and_clear_render_target();

        //if (tex.is_ready())
        //    texid = tex.get();

        //auto texxx = tmgr->load_unwrap<grx::color_rgb>("/home/ptrnine/Рабочий стол/kek.png");
        //texxx.set_load_significance(grx::texture_load_significance::high);

        vbo.bind_vao();
        m.use_program(t.base());
        m.set_uniform(t.base(), "MVP", cam->view_projection());
        m.set_uniform(t.base(), "texture0", 0);
        //texxx.activate_and_bind(0);
        bool wasready=false;
        if (tex.is_ready()) {
            wasready = true;
            auto texid = tex.get();
            //if (texid) {
                texid.activate_and_bind(0);
                texid.set_load_significance(grx::texture_load_significance::low);
            //}
            //std::cout << "Load..." << std::endl;
        }
        if (wasready)
            tex = tmgr->load_async_unwrap<grx::color_rgb>("/home/ptrnine/Рабочий стол/22.tga");

        /*
        if (tex.is_ready())
            texopt = tex.get();
        if (texopt)
            texopt->activate_and_bind(0);
            */

        vbo.draw(6);

        //for (auto& m : models)
        //    m.draw(cam->view_projection(), tech);

        wnd.present();

        wnd.update_input();

        //us.smooth();
    }

    return 0;
}

#include "graphics/grx_mesh_mgr.hpp"
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
#include <graphics/grx_debug.hpp>
#include <graphics/algorithms/grx_frustum_culling.hpp>
#include <core/fps_counter.hpp>
#include <graphics/grx_color_map.hpp>
#include <graphics/grx_texture.hpp>
#include <graphics/grx_texture_mgr.hpp>
#include <graphics/grx_shader.hpp>
#include <graphics/grx_shader_tech.hpp>
#include <graphics/grx_config_ext.hpp>
#include <core/keyboard_fuzzy_search.hpp>

#include <core/async.hpp>

int main() {
    std::ios_base::sync_with_stdio(false);
    core::config_manager cm;
    //grx::grx_shader_program_mgr m;

    auto wnd = grx::grx_window("wnd", {1600, 900});
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

    //auto tt = grx::grx_shader_tech(cm, "shader_tech_solid");
    auto tt = grx::grx_shader_tech(cm, "shader_tech_solid");

    grx::grx_mesh_mgr mm(cm);
    core::vector<grx::grx_mesh_instance> models;
    models.emplace_back(grx::grx_mesh_instance(mm, "cz805.dae"));
    models.back().set_debug_bone_aabb_draw(true);
    models.back().set_debug_aabb_draw(true);
    //for (float pos = 0; auto& m : models)
    //    m.move({pos += 20.f, 20.f, -20.f});

    //models.front().set_rotation({-90, 180, 30});
    //models.front().set_debug_bone_aabb_draw(true);
    //models.front().set_debug_aabb_draw(true);

    grx::grx_texture_set<3> texset;
    texset.set(0, tmgr->load_async<grx::color_rgb>("/home/ptrnine/Рабочий стол/22.tga"));

    //wnd.push_postprocess(grx::grx_postprocess(cm, "shader_gamma_correction"));
    wnd.push_postprocess(grx::grx_postprocess(cm, "shader_vhs1", [](grx::grx_shader_program& prg) {
        prg.get_uniform_unwrap<float>("time") =
            static_cast<float>(core::global_timer().measure_count());
    }));
    //wnd.push_postprocess({ m, cm, "shader_vhs2_texture", { "time" }, grx::postprocess_uniform_seconds()});
    //wnd.push_postprocess({ m, cm, "shader_vhs1_texture", { "time" }, grx::postprocess_uniform_seconds()});
    //wnd.push_postprocess({m, cm, "shader_gamma_correction", grx::uniform_pair{"gamma", 1/1.3f}});

    auto tech = grx::grx_shader_program::create_shared(cm, "shader_textured_basic");
    auto tmvp = tech->get_uniform_unwrap<glm::mat4>("MVP");
    auto ttexture = tech->get_uniform_unwrap<int>("texture0");

    core::update_smoother us(10, 0.8);
    core::fps_counter counter;
    core::timer timer;

    //grx::grx_texture<3> tx({4000, 2300});

    //auto tex = tmgr->load_async<grx::color_rgb>("/home/ptrnine/Рабочий стол/mabd2.png");
    //core::failure_opt<grx::grx_texture_id<3>> texopt;
    core::timer texdelay;

    while (!wnd.should_close()) {
        //if (texture2.is_ready()) {
        //    tx = texture2.get();
        //    core::printline("size: {}", tx.size());
        //}
        //auto texture2 = texture;

        counter.update();
        inp::inp_ctx().update();

        if (auto map = input_map.lock()) {
            if (map->GetBoolIsNew('R'))
                //textures.clear();
                models[0].play_animation("", true);
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
        tech->activate();
        tmvp = cam->view_projection();
        ttexture = 0;

        if (texset.get_resource(0).is_ready())
            texset.get_unwrap(0).bind_unit(0);
        //if (tex.is_ready()) {
        //    texdelay.reset();
        //    texopt = tex.get();
        //}

        //if (texopt) {
        //    texopt->bind_unit(0);
        //    texopt->set_load_significance(grx::texture_load_significance::low);
        //}

        //if (texopt && texdelay.measure_count() > 1.0) {
        //    texopt.reset();
        //    tex = tmgr->load_async<grx::color_rgb>("/home/ptrnine/Рабочий стол/22.tga");
        //}

        vbo.draw(6);

        for (auto& m : models)
            m.draw(cam->view_projection(), tt);

        wnd.present();

        wnd.update_input();

        //us.smooth();
    }

    return 0;
}

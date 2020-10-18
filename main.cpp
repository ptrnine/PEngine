
#define DEFERRED

#ifdef DEFERRED

#include "core/fps_counter.hpp"
#include <core/main.cpp>
#include <core/config_manager.hpp>
#include <graphics/grx_window.hpp>
#include <graphics/grx_camera.hpp>
#include <graphics/grx_camera_manipulator_fly.hpp>
#include <graphics/grx_shader_tech.hpp>
#include <graphics/grx_mesh_mgr.hpp>
#include <graphics/grx_mesh_instance.hpp>
#include <input/inp_input_ctx.hpp>
#include <graphics/grx_deferred_renderer_light.hpp>

using namespace grx;
using namespace core;

int pe_main(args_view args) {
    args.require_end();

    std::ios_base::sync_with_stdio(false);
    config_manager cm;

    auto wnd = grx_window("wnd", {1600, 900});
    wnd.set_pos({300, 0});
    wnd.make_current();

    wnd.push_postprocess(grx::grx_postprocess(cm, "shader_gamma_correction"));

    auto input_map = wnd.create_input_map();
    if (auto map = input_map.lock()) {
        auto keyboard = wnd.keyboard_id();
        map->MapBool('R', keyboard, gainput::KeyR);
        map->MapBool('Z', keyboard, gainput::KeyZ);
        map->MapBool('X', keyboard, gainput::KeyX);
        map->MapBool('C', keyboard, gainput::KeyC);
        map->MapBool('V', keyboard, gainput::KeyV);
    }

    auto cam = grx_camera::make_shared({0.f, 0.f, 5.f}, 16.f/9.f);
    cam->create_camera_manipulator<grx_camera_manipulator_fly>();
    cam->horizontal_fov(110.f);
    wnd.attach_camera(cam);

    auto ds_mgr = grx_ds_light_mgr::create_shared(wnd.size());
    ds_mgr->enable_debug_draw();
    //auto dir_light = ds_mgr->create_dir_light();
    ds_mgr->specular_power(32.f);
    ds_mgr->specular_intensity(4.f);
    //dir_light.direction(vec{0.05807f, -0.182f, 0.9816f}.normalize());
    //dir_light.ambient_intensity(0.00f);
    //dir_light.diffuse_intensity(1.0f);

    auto sl2 = ds_mgr->create_spot_light();
    sl2.beam_angle(40.f);
    sl2.ambient_intensity(0.f);
    sl2.diffuse_intensity(1.f);
    sl2.attenuation_constant(0.f);
    sl2.attenuation_quadratic(0.01f);
    sl2.color({0.8f, 0.1f, 0.4f});

    /*
    auto pl2 = ds_mgr->create_point_light();
    pl2.position({-96.07f, 8.655f, -34.93f});
    pl2.diffuse_intensity(1.0f);
    pl2.color({1.f, 0.f, 0.f});
    pl2.attenuation_constant(0.0f);
    pl2.attenuation_quadratic(0.001f);
    */

    auto shadow_sp = grx_shader_tech(cm, "shader_tech_csm_fr_shadow");
    auto geom_tech = grx_shader_tech(cm, "shader_tech_ds_geometry");
    auto ds_prog   = grx_shader_program::create_shared(cm, "shader_csm_ds");
    grx_mesh_mgr mm(cm);
    grx_mesh_instance mesh(mm, "cz805/cz805.dae");
    mesh.set_debug_aabb_draw(true);
    //grx_mesh_instance mesh(mm, "skate/skate.dae");
    //mesh.set_scale({3.f, 3.f, 3.f});
    //mesh.set_debug_aabb_draw(true);
    //mesh.set_debug_bone_aabb_draw(true);
    mesh.set_rotation({-90.f, 180.f, 0.f});

    sl2.position({-158.1f, -2.f, -10.f});
    sl2.direction({1.0f, 0.f, 0.f});

    timer timer;
    fps_counter counter;
    while (!wnd.should_close()) {
        inp::inp_ctx().update();

        if (auto map = input_map.lock()) {
            if (map->GetBoolIsNew('R'))
                mesh.play_animation("");
            if (map->GetBool('Z'))
                sl2.position(sl2.position() + vec3f{-0.2f, 0.f, 0.f});
            if (map->GetBool('X'))
                sl2.position(sl2.position() + vec3f{0.2f, 0.f, 0.f});
            if (map->GetBool('C'))
                sl2.beam_angle(sl2.beam_angle() - 0.2f);
            if (map->GetBool('V'))
                sl2.beam_angle(sl2.beam_angle() + 0.2f);
        }

        if (timer.measure() > core::seconds(1)) {
            printline("ray len: {}", sl2.max_ray_length());
            printline("FPS: {}, fov: {.3f} pos:{.4f} dir:{.4f}",
                    counter.get(), cam->horizontal_fov(), cam->position(), cam->directory());
            timer.reset();
        }

        sl2.position(cam->position());
        sl2.direction(cam->directory());

        mesh.update_animation_transforms();

        grx::grx_frustum_mgr().calculate_culling(cam->extract_frustum());

        ds_mgr->start_geometry_pass();
        mesh.draw(cam->view_projection(), geom_tech);
        //ds_mgr->debug_draw(cam->view_projection());
        //sss->activate();
        //sss->get_uniform_unwrap<glm::mat4>("MVP") = cam->view_projection() * spot_light.matttt();
        //spot_light.test_draw();

        ds_mgr->light_pass(ds_prog, cam->position(), cam->view_projection());

        wnd.bind_and_clear_render_target();
        ds_mgr->present(wnd.size());


        /*
        gbuf.start_geometry_pass();
        mesh.draw(cam->view_projection(), geom_tech);

        wnd.make_current();
        wnd.bind_and_clear_render_target();
        gbuf.start_lighting_pass();
        ds_mgr->draw(gbuf, ds_prog, cam->position());
        gbuf.end_lighting_pass();
        //gbuf.draw(ds_prog);
        */

        wnd.present();
        wnd.update_input();
        counter.update();
    }

    return 0;
}

#else

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
#include <graphics/grx_forward_renderer_light.hpp>
#include <graphics/grx_cascaded_shadow_mapping_tech.hpp>

#include <core/async.hpp>

#include <core/main.cpp>

int pe_main(core::args_view) {
    std::ios_base::sync_with_stdio(false);
    core::config_manager cm;

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
    cam->horizontal_fov(110.f);
    wnd.attach_camera(cam);

    auto tmgr = grx::grx_texture_mgr::create_shared(cm);

    auto shadow_sp     = grx::grx_shader_tech(cm, "shader_tech_csm_fr_shadow");
    auto light_sp_tech = grx::grx_shader_tech(cm, "shader_tech_csm_fr_textured");
    auto light_mgr     = grx::grx_forward_light_mgr::create_shared(wnd.size() * 4U, {1024, 1024});
    auto dir_light     = light_mgr->create_dir_light();
    //auto spot_light    = light_mgr->create_point_light();
    //light_sp_tech.skeleton()->get_uniform_unwrap<int>("texture0") = 0;
    //light_sp_tech.skeleton()->get_uniform_unwrap<int>("texture1") = 1;
    light_mgr->specular_power(32.f);
    light_mgr->specular_intensity(4.f);

    grx::grx_mesh_mgr mm(cm);

    grx::grx_mesh_instance mesh(mm, "cz805/cz805.dae");
    mesh.set_rotation({-90.f, 180.f, 0.f});
    grx::grx_mesh_pack mpack(mm, "basic/cube1.dae", 1000);
    for (size_t i = 1; i < mpack.count(); ++i) {
        mpack.move(i, {static_cast<float>(i) * 4.f, 0.f, 0.f});
    }

    //core::vector<grx::grx_mesh_instance> models;
    //bool flag = false;
    //for (auto _ : core::index_seq(1000)) {
        //models.emplace_back(grx::grx_mesh_instance(mm, "skate/skate.dae"));
        //if (flag)
    //        models.emplace_back(grx::grx_mesh_instance(mm, "basic/cube1.dae"));
        //else
        //    models.emplace_back(grx::grx_mesh_instance(mm, "cz805/cz805.dae"));
        //flag = !flag;
    //}

    //for (float pos = 0; auto& m : core::skip_view(models, 1)) {
        //m.set_debug_aabb_draw(true);
        //m.set_debug_bone_aabb_draw(true);
    //    m.move({pos += 3.5f, 0.f, 0.f});
    //}
    //models.front().set_debug_aabb_draw(true);
    //models.front().set_debug_bone_aabb_draw(true);
    //models.front().set_rotation({-90, 180, 0});

    wnd.push_postprocess(grx::grx_postprocess(cm, "shader_gamma_correction"));
    //wnd.push_postprocess(grx::grx_postprocess(cm, "shader_vhs1", [](grx::grx_shader_program& prg) {
    //    prg.get_uniform_unwrap<float>("time") =
    //        static_cast<float>(core::global_timer().measure_count());
    //}));

    core::fps_counter counter;
    core::timer timer;

    dir_light.direction(core::vec{0.9987f, 0.01335f, -0.04827f}.normalize());
    dir_light.ambient_intensity(0.00f);
    dir_light.diffuse_intensity(1.0f);

    while (!wnd.should_close()) {
        counter.update();
        inp::inp_ctx().update();

        if (auto map = input_map.lock()) {
            if (map->GetBoolIsNew('R')) {
                //textures.clear();
                //for (auto& m : models)
                    mesh.play_animation("", true);
            }
        }

        if (timer.measure() > core::seconds(1)) {
            core::printline("FPS: {}, fov: {.3f} pos:{.4f} dir:{.4f}",
                    counter.get(), cam->horizontal_fov(), cam->position(), cam->directory());
            timer.reset();
        }

        mpack.update_transforms();
        mesh.update_animation_transforms();
        //for (auto& m : models)
        //    m.update_animation_transforms();

        //spot_light.position(cam->position());
        //spot_light.position({-29.2f, 5.792f, 23.87f});
        //spot_light.direction(cam->directory());
        //spot_light.direction({0.881f, -0.1505f, -0.4484f});
        //spot_light.diffuse_intensity(1.0f);
        //spot_light.cutoff(0.9f);

        light_mgr->shadow_path(cam, core::span(&mesh, 1), shadow_sp);
        light_mgr->shadow_path(cam, mpack, shadow_sp);

        wnd.make_current(); // TODO: needless?
        wnd.bind_and_clear_render_target();

        light_mgr->setup_shadow_maps_to(light_sp_tech);

        light_mgr->setup_mvps(mesh.model_matrix(), light_sp_tech);
        light_mgr->setup_vps_instanced(light_sp_tech);
        mesh.draw(cam->view_projection(), light_sp_tech);
        mpack.draw(cam->view_projection(), light_sp_tech);
        //for (auto& m : models) {
        //    light_mgr->setup_mvps(m.model_matrix(), light_sp_tech);
        //    m.draw(cam->view_projection(), light_sp_tech);
        //}

        wnd.present();

        wnd.update_input();
    }

    return 0;
}

#endif

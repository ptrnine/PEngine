#include <random>

#include "core/fps_counter.hpp"
#include <core/main.cpp>
#include <core/config_manager.hpp>
#include <graphics/grx_window.hpp>
#include <graphics/grx_camera.hpp>
#include <graphics/grx_camera_manipulator_fly.hpp>
#include <graphics/grx_shader_tech.hpp>
#include <input/inp_input_ctx.hpp>
#include <graphics/grx_deferred_renderer_light.hpp>
#include <graphics/grx_cpu_mesh_group.hpp>
#include <graphics/grx_object.hpp>
#include <graphics/grx_object_mgr.hpp>

using namespace grx;
using namespace core;

int pe_main(args_view args) {
    args.require_end();

    std::ios_base::sync_with_stdio(false);
    config_manager cm;

    auto wnd = grx_window("wnd", {1600, 900});
    wnd.set_pos({300, 0});
    wnd.make_current();

    //wnd.push_postprocess(grx::grx_postprocess(cm, "shader_gamma_correction"));

    auto input_map = wnd.create_input_map();
    if (auto map = input_map.lock()) {
        auto keyboard = wnd.keyboard_id();
        map->MapBool('R', keyboard, gainput::KeyR);
        map->MapBool('Z', keyboard, gainput::KeyZ);
        map->MapBool('X', keyboard, gainput::KeyX);
        map->MapBool('C', keyboard, gainput::KeyC);
        map->MapBool('V', keyboard, gainput::KeyV);
    }

    auto cam = grx_camera::make_shared({0.f, 0.f, 5.f}, 16.f/9.f, 73.f, 0.1f);
    cam->create_camera_manipulator<grx_camera_manipulator_fly>();
    cam->horizontal_fov(110.f);
    wnd.attach_camera(cam);

    auto ds_mgr = grx_ds_light_mgr::create_shared(wnd.size());
    ds_mgr->enable_debug_draw();
    auto dir_light = ds_mgr->create_dir_light();
    ds_mgr->specular_power(12.f);
    ds_mgr->specular_intensity(1.5f);
    dir_light.direction(vec{0.8865f, -0.4057f, -0.2224f}.normalize());
    dir_light.ambient_intensity(0.5f);

    //auto sl2 = ds_mgr->create_spot_light();
    //sl2.beam_angle(40.f);
    //sl2.ambient_intensity(0.f);
    //sl2.diffuse_intensity(0.01f);
    //sl2.attenuation_constant(0.f);
    //sl2.attenuation_quadratic(0.01f);
    //sl2.color({0.8f, 0.1f, 0.4f});

    /*
    auto pl2 = ds_mgr->create_point_light();
    pl2.position({-96.07f, 8.655f, -34.93f});
    pl2.diffuse_intensity(1.0f);
    pl2.color({1.f, 0.f, 0.f});
    pl2.attenuation_constant(0.0f);
    pl2.attenuation_quadratic(0.001f);
    */

    /*
    auto cmap = load_color_map_unwrap<color_rgba>("/home/ptrnine/repo/PEngine/gamedata/models/glock19x/diffuse.png");
    auto cmap2 = cmap.get_resized({2048,2048});
    cmap2.gen_mipmaps();
    save_color_map_unwrap(cmap2, "/home/ptrnine/repo/PEngine/gamedata/models/glock19x/diffuse.petx");
    */

    auto shadow_sp = grx_shader_tech(cm, "shader_tech_csm_fr_shadow");
    auto geom_tech = grx_shader_tech(cm, "shader_tech_ds_geometry");
    auto si_prog =
        grx_shader_program::create_shared(cm, "shader_ds_geometry_textured_skeleton_instanced");
    auto i_prog =
        grx_shader_program::create_shared(cm, "shader_ds_geometry_textured_instanced");
    auto ds_prog   = grx_shader_program::create_shared(cm, "shader_csm_ds");
    //grx_mesh_mgr mm(cm, "mesh_mgr");
    //grx_mesh_instance mesh(mm, "cz805/cz805.dae");
    //grx_mesh_instance mesh(mm, "glock19x/glock19x.dae");
    //mesh.move({5.f, 0.f, 0.f});

    //auto ttmgr = grx_texture_mgr<4>::create_shared("manager");
    //auto obj = load_object<grx_cpu_boned_mesh_group_t>(ttmgr, "glock19x/glock19x.dae");
    //using obj_t = decltype(load_object<grx_cpu_boned_mesh_group_t>(ttmgr, ""));
    using obj_mgr_t = grx_object_type_to_mgr_t<true, grx_cpu_mesh_group_t>;

    auto obj_mgr = obj_mgr_t::create_shared("default");

    //obj_t obj;

    //auto obj = obj_mgr->load({"models_dir", "glock19x/glock19x.dae"});

    /*
    core::vector<decltype(obj_mgr->load({"", ""}))> instances;
    instances.reserve(200);
    for (int i = 0; i < 200; ++i) {
        instances.push_back(obj_mgr->load({"models_dir", "glock19x/glock19x.dae"}));
        instances.back().move({1.f * float(i), 0.f, 0.f});
        instances.back().rotation_angles({-90.f, 0.f, 0.f});
        //instances.back().play_animation(
        //    {"dh_idle", 1.0, false, grx_anim_permit::suspend});
    }
    **/
    auto obj2 = obj_mgr->load({"models_dir", "glock19x/glock19x.dae"});
    core::vector<decltype(obj2.create_instance())> instances;
    instances.reserve(200);
    for (int i = 0; i < 200; ++i) {
        instances.push_back(obj2.create_instance());
        instances.back().movable().move({1.f * float(i), 0.f, 0.f});
        instances.back().movable().rotation_angles({-90.f, 0.f, 0.f});
        //instances.back().animation_player().play_animation(
        //    {"dh_idle", 1.0, false, grx_anim_permit::suspend});
    }


    //obj2.rotation_angles({-90.f, 0.f, 0.f});
    //obj2.play_animation({"dh_idle", 1.0, false, grx_anim_permit::suspend});
    //auto obj_data = read_binary_file_unwrap("/home/ptrnine/Desktop/glock-pe.xxx");
    //deserializer_view deser(obj_data);
    //deser.read(obj);

    //while (obj.try_access() == nullptr);
    //serializer ser;
    //ser.write(*obj.try_access());
    //write_file_unwrap("/home/ptrnine/repo/PEngine/gamedata/models/glock19x/glock-pe2.xxx", ser.data());

    //mesh.set_scale({0.0525f, 0.0525f, 0.0525f});
    //mesh.set_debug_aabb_draw(true);
    //grx_mesh_instance mesh(mm, "skate/skate.dae");
    //mesh.set_scale({3.f, 3.f, 3.f});
    //mesh.set_debug_aabb_draw(true);
    //mesh.set_debug_bone_aabb_draw(true);
    //mesh.set_rotation({-90.f, 180.f, 0.f});

    //sl2.position({-158.1f, -2.f, -10.f});
    //sl2.direction({1.0f, 0.f, 0.f});

    //mesh.default_animation("idle");

    fps_counter counter;

    timer anim_timer;

    std::mt19937 mt;

    auto update_anim = [&input_map](grx_animation_player& animation_player) {
        if (auto map = input_map.lock()) {
            if (map->GetBoolIsNew('Z')) {
                string name = "dh_shot";
                if (auto anim = animation_player.current_animation()) {
                    if (anim->params.name == "dh_reload") {
                        if (anim->progress > 0.1 && anim->progress < 0.6)
                            name = "shot_empty";
                        else if (anim->progress < 0.75)
                            name += "_empty";
                    }
                }

                animation_player.play_animation(
                    grx_anim_params(name, 300ms),
                    grx_anim_hook(0.0, [](grx_animation_player& player, const grx_anim_params&) {
                        player.foreach_back([](grx_animation_player::anim_spec_t& anim) {
                            if (anim.progress <= 0.75 && anim.params.name == "dh_reload") {
                                anim.params.name += "_empty";
                                return true;
                            }
                            return false;
                        });
                    }));
            }
            if (map->GetBoolIsNew('R'))
                animation_player.play_animation(
                    {"dh_reload", 1600ms, true, grx_anim_permit::suspend, 200ms, 150ms});

            /*
            if (map->GetBool('Z'))
                sl2.position(sl2.position() + vec3f{-0.2f, 0.f, 0.f});
            if (map->GetBool('X'))
                sl2.position(sl2.position() + vec3f{0.2f, 0.f, 0.f});
            if (map->GetBool('C'))
                sl2.beam_angle(sl2.beam_angle() - 0.2f);
            if (map->GetBool('V'))
                sl2.beam_angle(sl2.beam_angle() + 0.2f);
                */
        }
    };

    while (!wnd.should_close()) {
        inp::inp_ctx().update();
        grx_ctx().update_states();

        //auto tickss = anim_timer.tick_count();
        //for (auto& i : instances) {
        //    update_anim(i);
        //    i.persistent_update(tickss);
        //}

        //          printline("ray len: {}", sl2.max_ray_length());

        /*
                sl2.position(cam->position());
                sl2.direction(cam->directory());
        */

        //mesh.update_animation_transforms();

        wnd.update_input();
        grx::grx_frustum_mgr().calculate_culling(cam->extract_frustum());

        ds_mgr->start_geometry_pass();
        //mesh.draw(cam->view_projection(), geom_tech);

        //obj.draw(cam->view_projection(), geom_tech);
        //obj2.persistent_update(anim_timer.tick_count());
        obj2.draw(cam->view_projection(), i_prog);
        //for (auto& obj : instances)
        //    obj.draw(cam->view_projection(), geom_tech);

        //if (state && tim.measure_count() > 0.5)
            //obj2.reset();

        //if (state && tim.measure_count() > 1.0) {
            //obj2 = core::optional{
          //      obj_mgr->load({"models_dir", "glock19x/glock19x.dae"}, load_significance_t::low)};
            //tim.reset();
        //}
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
        counter.update();

        LOG_UPDATE("FPS: {}, vertices: {}, fov: {.3f} pos:{.4f} dir:{.4f}",
                   static_cast<uint>(counter.get()),
                   grx_ctx().drawed_vertices(),
                   cam->horizontal_fov(),
                   cam->position(),
                   cam->directory());
    }

    return 0;
}


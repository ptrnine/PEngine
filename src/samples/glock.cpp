#include "graphics/algorithms/grx_frustum_culling.hpp"
#include "graphics/grx_context.hpp"
#include "graphics/grx_debug.hpp"
#include <core/fps_counter.hpp>
#include <core/main.cpp>
#include <graphics/grx_window.hpp>
#include <graphics/grx_camera.hpp>
#include <graphics/grx_camera_manipulator_fly.hpp>
#include <graphics/grx_shader_tech.hpp>
#include <graphics/grx_deferred_renderer_light.hpp>
#include <graphics/grx_object_mgr.hpp>
#include <graphics/grx_skybox.hpp>

using namespace grx;
using namespace core;

PE_DEFAULT_ARGS("--disable-file-logs");
PE_HELP("-i/--instances          - count of model instances\n"
        "-m/--mode               - draw mode (base, skeleton, instanced or skeleton_instanced)\n"
        "--debug                 - draw aabb boxes\n");

int pe_main(args_view args) {
    enum class mode {
        base, skeleton, instanced, skeleton_instanced
    };
    auto instance_count = args.by_key_default<size_t>({"--instances", "-i"}, 1);
    auto strmode = args.by_key_default<string>({"--mode", "-m"}, "base");

    auto str_to_mode = map<string, mode>{pair{"base", mode::base},
                                         pair{"skeleton", mode::skeleton},
                                         pair{"instanced", mode::instanced},
                                         pair{"skeleton_instanced", mode::skeleton_instanced}};

    if (!str_to_mode.contains(strmode))
        throw std::runtime_error("Invalid mode " + strmode);
    auto mode = str_to_mode[strmode];

    auto debug_draw = args.get("--debug");
    args.require_end();

    auto wnd = grx_window("wnd", {1600, 900});
    wnd.make_current();
    wnd.set_pos({300, 0});

    auto input_map = wnd.create_input_map();
    if (auto map = input_map.lock()) {
        auto keyboard = wnd.keyboard_id();
        map->MapBool('R', keyboard, gainput::KeyR);
        map->MapBool('Z', keyboard, gainput::KeyZ);
        map->MapBool('X', keyboard, gainput::KeyX);
        map->MapBool('C', keyboard, gainput::KeyC);
        map->MapBool('V', keyboard, gainput::KeyV);
    }

    auto cam = grx_camera::make_shared({0.f, 0.f, 0.f}, 16.f/9.f, 73.f, 0.1f);
    cam->create_camera_manipulator<grx_camera_manipulator_fly>();
    cam->horizontal_fov(110.f);
    wnd.attach_camera(cam);
    config_manager cm;
    cam->set_camera_animations(make_shared<grx_joint_animation_holder>(cm));

    auto ds_mgr = grx_ds_light_mgr::create_shared(wnd.size());
    ds_mgr->enable_debug_draw();
    auto dir_light = ds_mgr->create_dir_light();
    ds_mgr->specular_power(12.f);
    ds_mgr->specular_intensity(1.5f);
    dir_light.direction(vec{-0.5590571f, -0.1403328f, -0.817167f}.normalize());
    dir_light.ambient_intensity(0.5f);

    /*
    auto spot_light = ds_mgr->create_spot_light();
    spot_light.direction(vec{0.8865f, -0.4057f, -0.2224f}.normalize());
    spot_light.beam_angle(40.f);
    spot_light.ambient_intensity(0.f);
    spot_light.diffuse_intensity(1.0f);
    spot_light.attenuation_constant(0.f);
    spot_light.attenuation_quadratic(0.01f);
    spot_light.color({0.8f, 0.1f, 0.4f});
    LOG("max length: {}", spot_light.max_ray_length());
    */

    auto sp   = grx_shader_program::create_shared(cm, "shader_ds_geometry_textured");
    auto i_sp = grx_shader_program::create_shared(cm, "shader_ds_geometry_textured_instanced");
    auto s_sp = grx_shader_program::create_shared(cm, "shader_ds_geometry_textured_skeleton");
    auto is_sp =
        grx_shader_program::create_shared(cm, "shader_ds_geometry_textured_skeleton_instanced");
    auto ds_sp = grx_shader_program::create_shared(cm, "shader_csm_ds");

    using obj_mgr_t    = grx_object_type_to_mgr_t<false, grx_cpu_mesh_group_t>;
    using s_obj_mgr_t  = grx_object_type_to_mgr_t<false, grx_cpu_boned_mesh_group_t>;
    using i_obj_mgr_t  = grx_object_type_to_mgr_t<true, grx_cpu_mesh_group_t>;
    using si_obj_mgr_t = grx_object_type_to_mgr_t<true, grx_cpu_boned_mesh_group_t>;

    auto obj_mgr    = obj_mgr_t::create_shared("default");
    auto s_obj_mgr  = s_obj_mgr_t::create_shared("default");
    auto i_obj_mgr  = i_obj_mgr_t::create_shared("default");
    auto si_obj_mgr = si_obj_mgr_t::create_shared("default");

    auto obj_storage = vector<decltype(obj_mgr->load({"", ""}))>();
    obj_storage.reserve(instance_count);
    for (auto i : index_seq(instance_count)) {
        obj_storage.push_back(obj_mgr->load({"models_dir", "glock19x/glock19x.dae"}));
        obj_storage.back().move({0.1f + float(i) * 0.2f, -0.091f, -0.3f});
        obj_storage.back().scale({0.052941176f, 0.052941176f, 0.052941176f});
        obj_storage.back().rotation_angles({-90.f, 0.f, 0.f});
    }

    auto s_obj_storage = vector<decltype(s_obj_mgr->load({"", ""}))>();
    s_obj_storage.reserve(instance_count);
    for (auto i : index_seq(instance_count)) {
        s_obj_storage.push_back(s_obj_mgr->load({"models_dir", "glock19x/glock19x.dae"}));
        s_obj_storage.back().move({0.1f + float(i) * 0.2f, -0.091f, -0.3f});
        s_obj_storage.back().rotation_angles({-90.f, 0.f, 0.f});
        s_obj_storage.back().scale({0.052941176f, 0.052941176f, 0.052941176f});
        s_obj_storage.back().play_animation({"dh_idle", 1.0, false, grx_anim_permit::suspend});
    }

    auto i_obj  = i_obj_mgr->load({"models_dir", "glock19x/glock19x.dae"});
    auto si_obj = si_obj_mgr->load({"models_dir", "glock19x/glock19x.dae"});

    auto i_inst_storage = vector<decltype(i_obj.create_instance())>();
    i_inst_storage.reserve(instance_count);
    for (auto i : index_seq(instance_count)) {
        i_inst_storage.push_back(i_obj.create_instance());
        i_inst_storage.back().movable().move({0.1f + float(i) * 0.2f, -0.091f, -0.3f});
        i_inst_storage.back().movable().rotation_angles({-90.f, 0.f, 0.f});
        i_inst_storage.back().movable().scale({0.052941176f, 0.052941176f, 0.052941176f});
    }

    auto si_inst_storage = vector<decltype(si_obj.create_instance())>();
    si_inst_storage.reserve(instance_count);
    for (auto i : index_seq(instance_count)) {
        si_inst_storage.push_back(si_obj.create_instance());
        si_inst_storage.back().movable().move({0.1f + float(i) * 0.2f, -0.091f, -0.3f});
        si_inst_storage.back().movable().rotation_angles({-90.f, 0.f, 0.f});
        si_inst_storage.back().movable().scale({0.052941176f, 0.052941176f, 0.052941176f});
        si_inst_storage.back().animation_player().play_animation(
            {"dh_idle", 1.0, false, grx_anim_permit::suspend});
    }

    bool can_shot = true;
    auto update_anim = [&can_shot, &input_map, &cam](grx_animation_player& animation_player) {
        if (auto map = input_map.lock()) {
            if (map->GetBoolIsNew('Z') && can_shot) {
                bool shot_on_reload = false;
                string name = "dh_shot_new";
                if (auto anim = animation_player.current_animation()) {
                    if (anim->params.name == "dh_reload") {
                        shot_on_reload = true;
                        if (anim->progress > 0.1 && anim->progress < 0.6)
                            name = "shot_empty_new";
                        else if (anim->progress < 0.75)
                            name = name / replace("_new", "_empty_new");
                    }
                }
                animation_player.play_animation(
                    grx_anim_params(name, 400ms),
                    grx_anim_hook(0.0, [cam](grx_animation_player& player, const grx_anim_params&) {
                        cam->play_animation("cam_pistol_light_shot");
                        player.foreach_back([](grx_animation_player::anim_spec_t& anim) {
                            if (anim.progress <= 0.75 && anim.params.name == "dh_reload") {
                                anim.params.name += "_empty";
                                return true;
                            }
                            return false;
                        });
                    }));

                if (shot_on_reload)
                    can_shot = false;
            }
            if (map->GetBoolIsNew('R')) {
                animation_player.play_animation(
                    {"dh_reload", 1600ms, true, grx_anim_permit::suspend, 200ms, 150ms},
                    grx_anim_hook{0.999, [&](grx_animation_player&, const grx_anim_params&) {
                        can_shot = true;
                    }},
                    [cam](grx_animation_player&, const grx_anim_params&) {
                        cam->animation_player().suspend_animation("cam_pistol_reload");
                    },
                    [cam](grx_animation_player&, const grx_anim_params&) {
                        cam->animation_player().resume_animation("cam_pistol_reload");
                    });
                cam->play_animation("cam_pistol_reload");
            }
        }
    };

    auto skybox = grx_skybox(cm, "skybox_example");

    timer anim_timer;
    fps_counter fps;

    if (debug_draw)
        grx_aabb_debug().enable();

    vec3f last_pos;

    while (!wnd.should_close()) {
        inp::inp_ctx().update();
        grx_ctx().update_states();
        wnd.update_input();

        ds_mgr->start_geometry_pass();

        auto vp = cam->view_projection();

        s_obj_storage.front().direct_combine_transforms({cam->view()});

        auto anim_step = anim_timer.tick_count();
        switch (mode) {
        case mode::base:
            for (auto& obj : obj_storage)
                obj.draw(vp, sp);
            break;

        case mode::skeleton:
            for (auto& obj : s_obj_storage) {
                update_anim(obj);
                obj.persistent_update(anim_step);
                obj.draw(vp, s_sp);
            }
            break;

        case mode::instanced:
            i_obj.draw(vp, i_sp);
            break;

        case mode::skeleton_instanced:
            for (auto& inst : si_inst_storage) {
                update_anim(inst.animation_player());
                inst.persistent_update(anim_step);
            }
            si_obj.draw(vp, is_sp);
            break;
        }

        grx_aabb_debug().draw_and_clear(vp);

        ds_mgr->light_pass(ds_sp, cam->position(), vp);
        wnd.bind_and_clear_render_target();
        ds_mgr->present(wnd.size());

        if (auto map = input_map.lock()) {
            if (!map->GetBool('V'))
                last_pos = cam->position();
        }
        skybox.draw(last_pos, vp);

        wnd.present();

        grx_frustum_mgr().calculate_culling(cam->extract_frustum(),
                                            frustum_bits::csm_near | frustum_bits::csm_middle |
                                                frustum_bits::csm_far);

        fps.update();
        LOG_UPDATE("fps: {} verts: {}", fps.get(), grx_ctx().drawed_vertices());
        grx_ctx().update_states();
    }

    return 0;
}

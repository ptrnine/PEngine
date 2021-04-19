#include "graphics/algorithms/grx_frustum_culling.hpp"
#include "graphics/grx_context.hpp"
#include "graphics/grx_debug.hpp"
#include "graphics/grx_joint_animation.hpp"
#include <core/fps_counter.hpp>
#include <core/main.cpp>
#include <graphics/grx_window.hpp>
#include <graphics/grx_camera.hpp>
#include <graphics/grx_camera_manipulator_fly.hpp>
#include <graphics/grx_shader_tech.hpp>
#include <graphics/grx_deferred_renderer_light.hpp>
#include <graphics/grx_object_mgr.hpp>
#include <graphics/grx_skybox.hpp>
#include <core/avg_counter.hpp>
#include <ui/nuklear.hpp>

using namespace grx;
using namespace core;

PE_DEFAULT_ARGS("--disable-file-logs");

int pe_main(args_view args) {
    args.require_end();

    config_manager cm;

    auto wnd = grx_window::create_shared("wnd", {1600, 900});
    wnd->make_current();
    wnd->set_pos({300, 0});
    wnd->enable_luminance_calculation();
    wnd->enable_mouse_warp();
    auto ui = ui::ui_ctx(wnd);

    auto avg_exp = core::avg_counter<float>(200);

    wnd->push_postprocess(grx_postprocess(cm, "shader_hdr", [&wnd, &avg_exp](grx_shader_program& sp) {
        auto lum = std::clamp(std::sqrt(wnd->scene_luminance()), 0.05f, 1.f);
        auto lum_factor = inverse_lerp(0.05f, 1.f, lum);
        auto exposure = lerp(0.1f, 15.f, 1.f - lum_factor);
        avg_exp.update(exposure);
        LOG_UPDATE("luminance: {} exposure: {}", wnd->scene_luminance(), avg_exp.value());
        //LOG_UPDATE("exposure: {}", avg_exp.value());
        sp.get_uniform<float>("exposure") = avg_exp.value();
    }));
    /*
    wnd.push_postprocess(grx_postprocess(cm, "shader_vhs1", [tmr = timer()](grx_shader_program& vhs) {
        vhs.get_uniform<float>("time") = tmr.measure_count<float>();
    }));
    */

    auto input_map = wnd->create_input_map();
    if (auto map = input_map.lock()) {
        auto keyboard = wnd->keyboard_id();
        map->MapBool('R', keyboard, gainput::KeyR);
        map->MapBool('Z', keyboard, gainput::KeyZ);
        map->MapBool('X', keyboard, gainput::KeyX);
        map->MapBool('C', keyboard, gainput::KeyC);
        map->MapBool('V', keyboard, gainput::KeyV);
    }

    auto cam = grx_camera::make_shared({0.f, 0.f, 0.f}, 16.f/9.f, 73.f, 0.1f);
    cam->create_camera_manipulator<grx_camera_manipulator_fly>();
    cam->horizontal_fov(110.f);
    wnd->attach_camera(cam);
    auto joint_anim_holder = make_shared<grx_joint_animation_holder>(cm);
    cam->set_camera_animations(joint_anim_holder);

    auto ds_mgr = grx_ds_light_mgr::create_shared(wnd->size());
    ds_mgr->enable_debug_draw();
    auto dir_light = ds_mgr->create_dir_light();
    ds_mgr->specular_power(12.f);
    ds_mgr->specular_intensity(1.5f);
    dir_light.direction(vec{-0.5590571f, -0.1403328f, -0.817167f}.normalize());
    dir_light.ambient_intensity(0.1f);
    dir_light.diffuse_intensity(0.7f);

    auto s_sp = grx_shader_program::create_shared(cm, "shader_ds_geometry_textured_skeleton");
    auto ds_sp = grx_shader_program::create_shared(cm, "shader_csm_ds");

    using s_obj_mgr_t  = grx_object_type_to_mgr_t<false, grx_cpu_boned_mesh_group_t>;
    auto s_obj_mgr  = s_obj_mgr_t::create_shared("default");

    auto glock = s_obj_mgr->load({"models_dir", "glock19x/glock19x.dae"});
    glock.move({0.1f, -0.091f, -0.3f});
    glock.rotation_angles({-90.f, 0.f, 0.f});
    glock.scale({0.052941176f, 0.052941176f, 0.052941176f});
    glock.play_animation({"dh_idle", 1.0, false, grx_anim_permit::suspend});
    glock.setup_joint_animations(joint_anim_holder);

    auto x_avg = avg_counter<float>(40);
    auto y_avg = avg_counter<float>(40);

    bool can_shot = true;
    auto update_anim = [&can_shot, &input_map, &cam, &glock](grx_animation_player& animation_player) {
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
                    grx_anim_hook(0.0, [cam, &glock](grx_animation_player& player, const grx_anim_params&) {
                        cam->play_animation("cam_pistol_light_shot");
                        glock.animation_player()->play_animation("pistol_light_shot");
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
                    [cam, &glock](grx_animation_player&, const grx_anim_params&) {
                        cam->animation_player().suspend_animation("cam_pistol_reload");
                        glock.animation_player()->suspend_animation("pistol_reload");
                    },
                    [cam, &glock](grx_animation_player&, const grx_anim_params&) {
                        cam->animation_player().resume_animation("cam_pistol_reload");
                        glock.animation_player()->resume_animation("pistol_reload");
                    });
                cam->play_animation("cam_pistol_reload");
                glock.animation_player()->play_animation("pistol_reload");
            }
        }
    };

    auto draw_ui = [&ui, bg = vec{1.f, 1.f, 1.f, 1.f}]() mutable {
        if (ui.begin("Demo", nk_rect(50, 50, 230, 250),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;
            ui.layout_row_static(30, 80, 1);
            if (ui.button_label("button"))
                println("button pressed");

            ui.layout_row_dynamic(30, 2);
            if (ui.option_label("easy", op == EASY)) op = EASY;
            if (ui.option_label("hard", op == HARD)) op = HARD;

            ui.layout_row_dynamic(25, 1);
            ui.property_int("Compression:", 0, &property, 100, 10, 1);

            ui.layout_row_dynamic(20, 1);
            ui.label("background:", NK_TEXT_LEFT);
            ui.layout_row_dynamic(25, 1);
            if (ui.combo_begin_color(hdr_color_cut_to_u8(bg), vec{ui.widget_width(), 400})) {
                ui.layout_row_dynamic(120, 1);
                bg = ui.color_picker(bg, NK_RGBA);
                ui.layout_row_dynamic(25, 1);
                bg.r() = ui.propertyf("#R:", 0, bg.r(), 1.0f, 0.01f, 0.005f);
                bg.g() = ui.propertyf("#G:", 0, bg.g(), 1.0f, 0.01f, 0.005f);
                bg.b() = ui.propertyf("#B:", 0, bg.b(), 1.0f, 0.01f, 0.005f);
                bg.a() = ui.propertyf("#A:", 0, bg.a(), 1.0f, 0.01f, 0.005f);
                ui.combo_end();
            }
        }
        ui.end();
    };

    auto skybox = grx_skybox(cm, "skybox_example");

    timer anim_timer;
    fps_counter fps;

    vec3f last_pos;

    grx_joint_animation_player glock_joint_anim;

    grx_aabb_debug().enable();

    while (!wnd->should_close()) {
        inp::inp_ctx().update();
        grx_ctx().update_states();
        wnd->update_input();

        ui.new_frame();

        ds_mgr->start_geometry_pass();

        auto vp = cam->view_projection();

        /* Update floating weapon hud */
        x_avg.update(cam->ypr_speed().x() * 0.004f);
        y_avg.update(cam->ypr_speed().y() * 0.006f);
        glock.direct_combine_transforms(
            {glm::translate(glm::mat4(1.f), {x_avg.value(), -y_avg.value(), 0.f}),
             cam->view()});

        auto anim_step = anim_timer.tick_count();
        update_anim(glock);
        glock.persistent_update(anim_step);
        glock.update_joint_animations();
        glock.draw(vp, s_sp);

        ds_mgr->light_pass(ds_sp, cam->position(), vp);
        wnd->bind_and_clear_render_target();
        ds_mgr->present(wnd->size());

        if (auto map = input_map.lock()) {
            if (!map->GetBool('V'))
                last_pos = cam->position();
            if (map->GetBoolIsNew('C'))
                wnd->disable_mouse_warp();
            if (map->GetBoolWasDown('C'))
                wnd->enable_mouse_warp();
        }
        skybox.draw(last_pos, vp);

        wnd->present();

        if (auto map = input_map.lock(); map && map->GetBool('C'))
            draw_ui();

        ui.render();
        wnd->swap_buffers();

        grx_frustum_mgr().calculate_culling(cam->extract_frustum(),
                                            frustum_bits::csm_near | frustum_bits::csm_middle |
                                                frustum_bits::csm_far);

        fps.update();

        //LOG_UPDATE("fps: {} pos: {} verts: {}", fps.get(), cam->position(), grx_ctx().drawed_vertices());
        grx_ctx().update_states();
    }

    return 0;
}

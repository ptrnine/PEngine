#include <core/main.cpp>
#include <graphics/grx_window.hpp>
#include <graphics/grx_camera.hpp>
#include <graphics/grx_camera_manipulator_fly.hpp>
#include <graphics/grx_vbo_tuple.hpp>
#include <graphics/grx_texture.hpp>
#include <graphics/grx_mesh_gen.hpp>

using namespace core;
using namespace grx;

int pe_main(args_view args) {
    /* Require no arguments */
    args.require_end();

    /* Create window */
    auto wnd = grx_window("vbo_sample.cpp", {800, 600});
    wnd.make_current();

    /* Create camera with 5.0 units z-axis, 8/6 aspect ratio */
    auto cam = grx_camera::make_shared({0.f, 0.f, 5.f}, 8.f/6.f);

    /* Create camera manipulator for "fly" camera control */
    cam->create_camera_manipulator<grx_camera_manipulator_fly>();

    /* Attach camera to window */
    wnd.attach_camera(cam);

    /* Create vbo tuple, generate cube */
    mesh_vbo_t vbos;
    vbos.set_data(gen_cube());

    /* Create RGBA color map 512x512 */
    grx_color_map_rgba image({512, 512});

    /* Function for filling squares on UV */
    auto fill_side = [&](vec2f start, color_rgba color) {
        auto quad_size  = static_cast<vec2u>(static_cast<vec2f>(image.size()) * (1/3.0));
        auto quad_start = static_cast<vec2u>(static_cast<vec2f>(image.size()) * (start));
        for (auto v : dimensional_seq(quad_size))
            image[v + quad_start] = color;
    };
    /*
     * UV scheme:
     * [0] [#] [#]
     * [1] [3] [#]
     * [2] [4] [5]
     */
    fill_side({  0.f, 0.f  }, {255,   0, 255, 255}); // [0]
    fill_side({  0.f, 1/3.f}, {255, 127, 255, 255}); // [1]
    fill_side({  0.f, 2/3.f}, {255, 200, 255, 255}); // [2]
    fill_side({1/3.f, 1/3.f}, {  0, 255, 100, 255}); // [3]
    fill_side({1/3.f, 2/3.f}, {  0, 255, 255, 255}); // [4]
    fill_side({2/3.f, 2/3.f}, {127, 255, 255, 255}); // [5]

    /* Create texture from image */
    grx_texture texture = image;

    /* Vertex and fragment shaders */
    auto shader = grx_shader_program::create_shared(
        grx_shader<shader_type::vertex>(
            "uniform mat4 MVP;"
            "in vec3 position_ms;"
            "in vec2 uv_in;"
            "out vec2 uv;"
            "void main() {"
            "    gl_Position = MVP * vec4(position_ms, 1.0);"
            "    uv          = uv_in;"
            "}"
        ),
        grx_shader<shader_type::fragment>(
            "uniform sampler2D sampler0;"
            "in vec2 uv;"
            "out vec4 color;"
            "void main() {"
            "    color = texture(sampler0, uv);"
            "}"
        )
    );

    /* Get Matrix-View-Projection matrix uniform */
    auto MVP = shader->get_uniform<glm::mat4>("MVP");

    while (!wnd.should_close()) {
        /* Update input context */
        inp::inp_ctx().update();

        wnd.bind_and_clear_render_target();

        /* Bind our cube, shader, texture, setup MVP and draw */
        vbos.bind_vao();
        shader->activate();
        MVP = cam->view_projection() * glm::mat4(1.f);
        texture.bind_unit<0>();
        vbos.draw(*vbos.indices_count());

        /* Present screen */
        wnd.present();
        wnd.update_input();
    }

    return 0;
}

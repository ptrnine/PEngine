#include <filesystem>
#include <core/config_manager.hpp>
#include <core/platform_dependent.hpp>
#include <input/inp_input_ctx.hpp>
#include <graphics/grx_vbo_tuple.hpp>
#include <graphics/grx_window.hpp>
#include <graphics/grx_camera.hpp>
#include <graphics/grx_camera_manipulator_fly.hpp>
#include <graphics/grx_texture.hpp>

using namespace core;

int main() {
    auto window = grx::grx_window("wnd", {800, 600}); // NOLINT
    window.make_current();

    auto camera = grx::grx_camera::make_shared({0.f, 0.f, 5.f}, 4.f/3.f);
    camera->create_camera_manipulator<grx::grx_camera_manipulator_fly>();
    window.attach_camera(camera);

    auto texture_path = DEFAULT_CFG_PATH() / ".." / config_section::direct_read().read_unwrap<string>("textures_dir");

    auto texture = grx::load_texture_unwrap(texture_path / "cake.jpg");
    auto shader  = grx::grx_shader_program::create_shared(
        grx::grx_shader<grx::shader_type::vertex>(
            "uniform mat4 MVP;"
            "in vec3 position_ms;"
            "out vec2 uv;"
            "void main() {"
            "    gl_Position = MVP * vec4(position_ms, 1.0);"
            "    uv = (position_ms.xy + vec2(1.0, 1.0)) * 0.5;"
            "}"
        ),
        grx::grx_shader<grx::shader_type::fragment>(
            "uniform sampler2D sampler0;"
            "in vec2 uv;"
            "out vec4 color;"
            "void main() {"
            "    color = texture(sampler0, uv);"
            "}"
        )
    );
    auto MVP = shader->get_uniform_unwrap<glm::mat4>("MVP");

    grx::grx_vbo_tuple<grx::vbo_vector_vec3f> quad;
    quad.set_data<0>({
        {0.f, 1.f, 0.f},
        {0.f, 0.f, 0.f},
        {1.f, 1.f, 0.f},
        {1.f, 1.f, 0.f},
        {0.f, 0.f, 0.f},
        {1.f, 0.f, 0.f}
    });

    while (!window.should_close()) {
        inp::inp_ctx().update();
        window.bind_and_clear_render_target();

        quad.bind_vao();
        shader->activate();
        MVP = camera->view_projection() * glm::mat4(1.f);
        texture.bind_unit<0>();
        quad.draw(18); // NOLINT

        window.present();
        window.update_input();
    }

    return 0;
}


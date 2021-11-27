#pragma once

#include "grx_color_map.hpp"
#include "grx_vbo_tuple.hpp"
#include "grx_mesh_gen.hpp"
#include "grx_shader.hpp"

namespace grx {

namespace details {
    uint load_skybox(const core::array<grx_float_color_map_rgb, 6>& maps);
    void skybox_bind_texture(uint id);
}

enum class skybox_transforms {
    mirror_down = 0,
    rotate_left,
    rotate_right,
    rotate_180,
    flip_horizontal,
    flip_vertical
};

class grx_skybox {
public:
    grx_skybox(const core::config_manager& config_mgr, const core::string& section) {
        using namespace core;
        constexpr auto load_map = [](const config_manager& cm, const string& s, string_view name) {
            auto [str_path, transforms] = cm.read<tuple<string, core::vector<skybox_transforms>>>(s, name);
            auto path = cfg_path("textures_dir", str_path).absolute();
            auto map = load_color_map<float_color_rgb>(path);

            for (auto& transform : transforms) {
                switch (transform) {
                case skybox_transforms::mirror_down:
                    map = map.mirrored_down();
                    break;
                case skybox_transforms::rotate_left:
                    map = map.rotated_left();
                    break;
                case skybox_transforms::rotate_right:
                    map = map.rotated_right();
                    break;
                case skybox_transforms::rotate_180:
                    map = map.rotated_180();
                    break;
                case skybox_transforms::flip_horizontal:
                    map.flip_horizontal();
                    break;
                case skybox_transforms::flip_vertical:
                    map.flip_vertical();
                    break;
                }
            }

            return map;
        };

        auto maps = core::array{
            load_map(config_mgr, section, "right"),
            load_map(config_mgr, section, "left"),
            load_map(config_mgr, section, "up"),
            load_map(config_mgr, section, "down"),
            load_map(config_mgr, section, "front"),
            load_map(config_mgr, section, "back")
        };

        _cube_map = details::load_skybox(maps);
        _box.set_data<0>({
                { -1.0f,  1.0f, -1.0f },
                { -1.0f, -1.0f, -1.0f },
                {  1.0f, -1.0f, -1.0f },
                {  1.0f, -1.0f, -1.0f },
                {  1.0f,  1.0f, -1.0f },
                { -1.0f,  1.0f, -1.0f },

                { -1.0f, -1.0f,  1.0f },
                { -1.0f, -1.0f, -1.0f },
                { -1.0f,  1.0f, -1.0f },
                { -1.0f,  1.0f, -1.0f },
                { -1.0f,  1.0f,  1.0f },
                { -1.0f, -1.0f,  1.0f },

                {  1.0f, -1.0f, -1.0f },
                {  1.0f, -1.0f,  1.0f },
                {  1.0f,  1.0f,  1.0f },
                {  1.0f,  1.0f,  1.0f },
                {  1.0f,  1.0f, -1.0f },
                {  1.0f, -1.0f, -1.0f },

                { -1.0f, -1.0f,  1.0f },
                { -1.0f,  1.0f,  1.0f },
                {  1.0f,  1.0f,  1.0f },
                {  1.0f,  1.0f,  1.0f },
                {  1.0f, -1.0f,  1.0f },
                { -1.0f, -1.0f,  1.0f },

                { -1.0f,  1.0f, -1.0f },
                {  1.0f,  1.0f, -1.0f },
                {  1.0f,  1.0f,  1.0f },
                {  1.0f,  1.0f,  1.0f },
                { -1.0f,  1.0f,  1.0f },
                { -1.0f,  1.0f, -1.0f },

                { -1.0f, -1.0f, -1.0f },
                { -1.0f, -1.0f,  1.0f },
                {  1.0f, -1.0f, -1.0f },
                {  1.0f, -1.0f, -1.0f },
                { -1.0f, -1.0f,  1.0f },
                {  1.0f, -1.0f,  1.0f }}
                );

        _sp = grx_shader_program::create_shared(
            grx_shader<shader_type::vertex>("layout (location = 0) in  vec3 position_ms;",
                                            "out vec3 tex_coords;",
                                            "uniform mat4 MVP;",
                                            "void main() {",
                                            "    tex_coords = position_ms;",
                                            "    vec4 pos = (MVP * vec4(position_ms, 1.0)).xyww;",
                                            "    pos.z *= 0.999999;",
                                            "    gl_Position = pos;",
                                            "}"),
            grx_shader<shader_type::fragment>("in  vec3 tex_coords;",
                                              "out vec4 color;",
                                              "uniform samplerCube skybox;",
                                              "void main() {",
                                              "    color = texture(skybox, tex_coords);",
                                              "}"));
    }

    void draw(const vec3f& position, const glm::mat4& vp) {
        auto model_mat = glm::translate(glm::mat4(1.f), core::to_glm(position));

        _box.bind_vao();
        _sp->activate();
        _sp->get_uniform<glm::mat4>("MVP") = vp * model_mat;
        _sp->get_uniform<int>("skybox") = 0;
        details::skybox_bind_texture(_cube_map);
        _box.draw(36);
    }

private:
    grx_vbo_tuple<vbo_vector_vec3f>      _box;
    core::shared_ptr<grx_shader_program> _sp;
    uint _cube_map;
};

}

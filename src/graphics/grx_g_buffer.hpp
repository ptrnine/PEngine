#pragma once

#include "grx_types.hpp"
#include "grx_vbo_tuple.hpp"
#include "grx_shader_tech.hpp"

namespace grx {

namespace details {
    uint gen_g_buffer(vec2i size, uint* attachments);
    void bind_g_buffer(uint g_buffer);
    void bind_and_clear_g_buffer(uint g_buffer);
    void activate_g_buffer_textures(uint* attachments);
}

class grx_g_buffer {
public:
    enum {
        gbuf_albedo = 0,
        gbuf_normal,
        gbuf_position_depth,
        gbuf_depth,
        gbuf_final,
        gbuf_SIZE
    };

    grx_g_buffer(vec2u isize): size(isize)  { // NOLINT
        fbo = details::gen_g_buffer(static_cast<vec2i>(size), attachments.data());
        quad.set_data<0>({
                {-1.0f, -1.0f, 0.0f },
                { 1.0f, -1.0f, 0.0f },
                {-1.0f,  1.0f, 0.0f },
                {-1.0f,  1.0f, 0.0f },
                { 1.0f, -1.0f, 0.0f },
                { 1.0f,  1.0f, 0.0f }
        });
    }

    void bind_geometry_pass();
    void bind_stencil_path();
    void bind_lighting_pass();
    void bind_present_pass();
    void clear_result_attachment();
    //void end_lighting_pass();

    void bind() {
        details::bind_g_buffer(fbo);
    }

    void bind_and_clear() {
        details::bind_and_clear_g_buffer(fbo);
    }

    void activate_textures() {
        details::activate_g_buffer_textures(attachments.data());
    }

    void draw(const core::shared_ptr<grx_shader_program>& program) {
        _draw(program, quad);
    }

    template <typename VboTupleT>
    void draw(const core::shared_ptr<grx_shader_program>& program, VboTupleT& vbo_tuple) {
        _draw(program, vbo_tuple);
    }

    void draw(const grx_shader_tech& tech, grx_mesh_type type) {
        draw(tech, type, quad);
    }

    template <typename VboTupleT>
    void draw(const grx_shader_tech& tech, grx_mesh_type type, VboTupleT& vbo_tuple) {
        switch (type) {
            case grx_mesh_type::Basic:
                _draw(tech.base(), vbo_tuple);
                break;
            case grx_mesh_type::Skeleton:
                _draw(tech.skeleton(), vbo_tuple);
                break;
            case grx_mesh_type::Instanced:
                _draw(tech.instanced(), vbo_tuple);
                break;
        }
    }

    vec2u fbo_size() const {
        return size;
    }

private:
    template <typename VboTupleT>
    void _draw(const core::shared_ptr<grx_shader_program>& program, VboTupleT& vbo_tuple) {
        program->activate();

        if (auto u = program->get_uniform<int>("position_depth"))
            *u = gbuf_position_depth;
        if (auto u = program->get_uniform<int>("normal"))
            *u = gbuf_normal;
        if (auto u = program->get_uniform<int>("albedo"))
            *u = gbuf_albedo;

        vbo_tuple.bind_vao();
        if (auto indices_count = vbo_tuple.indices_count())
            vbo_tuple.draw(*indices_count);
        else
            vbo_tuple.draw(18);

    }

    uint fbo;
    uint tmp_texture;
    grx_vbo_tuple<vbo_vector_vec3f> quad;
    core::array<uint, gbuf_SIZE>    attachments;
    vec2u                           size;
};

}

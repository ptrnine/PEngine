#pragma once

#include "grx_vbo_tuple.hpp"

namespace grx {
    //mesh_buf_t gen_cube(vec3f size = {1.f, 1.f, 1.f});
    grx_vbo_tuple<vbo_vector_indices, vbo_vector_vec3f>
    gen_light_sphere();

    template <size_t vertices_pos = 1, VboData... Ts>
    void vbo_setup_as_box(grx_vbo_tuple<Ts...>& vbo) {
        vbo.template set_nth_type<vbo_vector_indices>(
                {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
                 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35});

        vbo.template set_data<vertices_pos>(
                {{1.f, -1.f, -1.f},  {-1.f, -1.f, -1.f}, {-1.f, 1.f, -1.f},
                 {1.f, -1.f, -1.f},  {-1.f, 1.f, -1.f},  {1.f, 1.f, -1.f},

                 {1.f, -1.f, 1.f},   {1.f, -1.f, -1.f},  {1.f, 1.f, -1.f},
                 {1.f, -1.f, 1.f},   {1.f, 1.f, -1.f},   {1.f, 1.f, 1.f},

                 {-1.f, -1.f, 1.f},  {1.f, -1.f, 1.f},   {1.f, 1.f, 1.f},
                 {-1.f, -1.f, 1.f},  {1.f, 1.f, 1.f},    {-1.f, 1.f, 1.f},

                 {-1.f, -1.f, -1.f}, {-1.f, -1.f, 1.f},  {-1.f, 1.f, 1.f},
                 {-1.f, -1.f, -1.f}, {-1.f, 1.f, 1.f},   {-1.f, 1.f, -1.f},

                 {1.f, 1.f, -1.f},   {-1.f, 1.f, -1.f},  {-1.f, 1.f, 1.f},
                 {1.f, 1.f, -1.f},   {-1.f, 1.f, 1.f},   {1.f, 1.f, 1.f},

                 {1.f, -1.f, 1.f},   {-1.f, -1.f, 1.f},  {-1.f, -1.f, -1.f},
                 {1.f, -1.f, 1.f},   {-1.f, -1.f, -1.f}, {1.f, -1.f, -1.f}});
    }
} // namespace grx


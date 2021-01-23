#pragma once

#include <core/helper_macros.hpp>
#include <core/types.hpp>
#include <core/vec.hpp>
#include <glm/mat4x4.hpp>
#include "grx_shader_mgr.hpp"

namespace grx
{
    class grx_mesh_instance;
    class grx_mesh_pack;
    class grx_camera;
    class grx_shader_tech;

    class grx_cascade_shadow_map_tech {
    public:
        struct uniforms_t {
            uniforms_t(grx_shader_program& sp):
                light_mvps (sp.get_uniform<core::span<glm::mat4>>("light_MVP")),
                shadow_maps(sp.get_uniform<core::span<int>>("shadow_map")),
                csm_end_cs (sp.get_uniform<core::span<float>>("csm_end_cs")) {}

            grx_uniform<core::span<glm::mat4>> light_mvps;
            grx_uniform<core::span<int>>       shadow_maps;
            grx_uniform<core::span<float>>     csm_end_cs;
        };

        struct ortho_projection {
            float left, right, bottom, top, near, far;
            TO_TUPLE_IMPL(left, right, bottom, top, near, far)
        };

        grx_cascade_shadow_map_tech(const core::vec2u& size, size_t maps_count, const core::vector<float>& z_bounds);
        grx_cascade_shadow_map_tech(const core::vec2u& size): grx_cascade_shadow_map_tech(size, 3, {1.1f, 70.f, 200.f, 1006.f}) {}
        ~grx_cascade_shadow_map_tech();

        void set_z_bounds(const core::vector<float>& values);
        void cover_view(const core::shared_ptr<grx_camera>& camera, const core::vec3f& light_direction);
        void draw(const core::shared_ptr<grx_shader_program>& shader, grx_mesh_instance& mesh_instance);
        void bind_framebuffer();
        void bind_shadow_maps(int start = 2);
        void setup(const core::shared_ptr<grx_shader_program>& shader_program, const glm::mat4& object_matrix);
        void setup_instanced(const core::shared_ptr<grx_shader_program>& shader_program);

        void culling_stage(const grx_camera& camera) const;
        void shadow_path(core::span<grx_mesh_instance> objects,
                         const grx_shader_tech&        tech) const;
        void shadow_path(grx_mesh_pack&         mesh_pack,
                         const grx_shader_tech& tech) const;

    private:
        core::vec2u                    _size;
        uint                           _fbo;

        // TODO: Move to struct this shit:
        core::vector<uint>             _shadow_maps;
        core::vector<ortho_projection> _ortho_projections;
        core::vector<glm::mat4>        _light_projections;
        core::vector<float>            _z_bounds;
        core::vector<int>              _gl_texture_nums;
        core::vector<float>            _csm_end_cs;
        glm::mat4                      _light_view;

        core::hash_map<grx_shader_program*, uniforms_t> _cached_uniforms;

        float _z_camera_shift = 100.f;

    public:
        void  set_z_camera_shift(float value) { _z_camera_shift = value; }
        [[nodiscard]]
        float z_camera_shift() const { return _z_camera_shift; }
    };

} // namespace grx


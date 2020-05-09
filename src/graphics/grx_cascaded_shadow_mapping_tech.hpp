#include <core/helper_macros.hpp>
#include <core/types.hpp>
#include <core/vec.hpp>
#include <glm/mat4x4.hpp>

namespace grx
{
    class grx_cascade_shadow_map_tech {
    public:
        struct ortho_projection {
            float left, right, bottom, top, near, far;
            TO_TUPLE_IMPL(left, right, bottom, top, near, far)
        };

        grx_cascade_shadow_map_tech(const core::vec2u& size, size_t maps_count, const core::vector<float>& z_bounds);
        grx_cascade_shadow_map_tech(const core::vec2u& size): grx_cascade_shadow_map_tech(size, 3, {1.f, 100.f, 250.f, 1500.f}) {}
        ~grx_cascade_shadow_map_tech();

        void set_z_bounds(const core::vector<float>& values);
        void cover_view(const glm::mat4& camera_view, float field_of_view, const core::vec3f& light_direction);

    private:
        core::vec2u                    _size;
        uint                           _fbo;
        core::vector<uint>             _shadow_maps;
        core::vector<ortho_projection> _ortho_projections;
        core::vector<float>            _z_bounds;
    };

} // namespace grx


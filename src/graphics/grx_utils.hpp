#pragma once

#include "graphics/grx_types.hpp"

namespace grx_utils {
    core::string collada_bake_bind_shape_matrix(core::string_view data);
    grx::grx_aabb_frustum_planes_fast extract_frustum(const glm::mat4& view_projection);
}

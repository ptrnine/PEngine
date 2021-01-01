#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/quaternion.hpp>

#include <core/types.hpp>
#include <core/vec.hpp>
#include <core/container_extensions.hpp>
#include <core/helper_macros.hpp>
#include <core/serialization.hpp>

namespace grx {
    using core::vec;
    using core::vec2f;
    using core::vec2d;
    using core::vec2i;
    using core::vec2u;
    using core::vec3f;
    using core::vec3d;
    using core::vec3i;
    using core::vec3u;
    using core::vec4f;
    using core::vec4d;
    using core::vec4i;
    using core::vec4u;
    using core::vec2;
    using core::vec3;
    using core::vec4;

    enum class shader_program_id_t : int  {};
    enum class shader_effect_id_t  : int  {};
    enum class uniform_id_t        : int  {};
    //enum class texture_id_t        : uint {};

    using color_r    = core::vec<uint8_t, 1>;
    using color_rg   = core::vec<uint8_t, 2>;
    using color_rgb  = core::vec<uint8_t, 3>;
    using color_rgba = core::vec<uint8_t, 4>;

    using float_color_r    = core::vec<float, 1>;
    using float_color_rg   = core::vec<float, 2>;
    using float_color_rgb  = core::vec<float, 3>;
    using float_color_rgba = core::vec<float, 4>;

    enum class grx_load_significance {
        low = 0, medium, high
    };

    enum class grx_color_fmt {
        RGB = 0, SRGB, RGB16, RGB16F, RGB32F
    };

    enum class grx_filtering {
        Linear = 0, Nearest
    };

    enum class grx_mesh_type {
        Basic = 0, Skeleton, Instanced
    };

    struct grx_aabb {
        PE_SERIALIZE(min, max)

        static grx_aabb maximized() {
            return grx_aabb{
                {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
                {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()}
            };
        }

        core::vec3f min;
        core::vec3f max;

        void merge(const grx_aabb& aabb) {
            min.x() = min.x() < aabb.min.x() ? min.x() : aabb.min.x();
            min.y() = min.y() < aabb.min.y() ? min.y() : aabb.min.y();
            min.z() = min.z() < aabb.min.z() ? min.z() : aabb.min.z();
            max.x() = max.x() > aabb.max.x() ? max.x() : aabb.max.x();
            max.y() = max.y() > aabb.max.y() ? max.y() : aabb.max.y();
            max.z() = max.z() > aabb.max.z() ? max.z() : aabb.max.z();
        }

        [[nodiscard]]
        grx_aabb get_merged(const grx_aabb& aabb) const {
            grx_aabb result = *this;
            result.merge(aabb);
            return result;
        }

        void transform(const glm::mat4& matrix) {
            auto aabb_points = core::array<glm::vec4, 8>{
                glm::vec4{min.x(), min.y(), min.z(), 1.f},
                glm::vec4{min.x(), max.y(), min.z(), 1.f},
                glm::vec4{max.x(), min.y(), min.z(), 1.f},
                glm::vec4{max.x(), max.y(), min.z(), 1.f},
                glm::vec4{min.x(), min.y(), max.z(), 1.f},
                glm::vec4{min.x(), max.y(), max.z(), 1.f},
                glm::vec4{max.x(), min.y(), max.z(), 1.f},
                glm::vec4{max.x(), max.y(), max.z(), 1.f}
            };

            min = core::from_glm(matrix * aabb_points.front()).xyz();
            max = min;

            for (auto& point : core::skip_view(aabb_points, 1)) {
                auto transformed = core::from_glm(matrix * point).xyz();
                merge(grx_aabb{transformed, transformed});
            }
        }
    };

    struct grx_aabb_fast {
        PE_SERIALIZE(min, max)

        grx_aabb_fast() = default;
        grx_aabb_fast(const core::vec3f& imin, const core::vec3f& imax):
            min{imin.x(), imin.y(), imin.z(), 1.0},
            max{imax.x(), imax.y(), imax.z(), 1.0} {}

        grx_aabb_fast(const grx_aabb& aabb) {
            min.xyz(aabb.min);
            max.xyz(aabb.max);
        }

        [[nodiscard]]
        grx_aabb aabb() const {
            return grx_aabb{min.xyz(), max.xyz()};
        }

        core::vec4f min = {0.f, 0.f, 0.f, 0.f};
        core::vec4f max = {0.f, 0.f, 0.f, 0.f};
    };

    struct grx_aabb_frustum_planes_fast {
        struct names_t {
            core::vec4f left;
            core::vec4f right;
            core::vec4f bottom;
            core::vec4f top;
            core::vec4f near;
            core::vec4f far;
        };

        union {
            names_t                     as_names;
            core::array<core::vec4f, 6> as_array;
        };
    };

    /*
    struct grx_texture {
        uint width = 0, height = 0, channels = 0;
        texture_id_t id = static_cast<texture_id_t>(std::numeric_limits<uint>::max());

        TO_TUPLE_IMPL(width, height, channels, id)
    };
    */

    template <grx_color_fmt ColorFmt, grx_filtering Filtering>
    struct grx_render_target_settings {
        using grx_render_target_settings_check = void;
        static constexpr grx_color_fmt color_fmt = ColorFmt;
        static constexpr grx_filtering filtering = Filtering;
    };

    template <typename T>
    concept RenderTargetSettings = requires { typename T::grx_render_target_settings_check; };
}

namespace core {
    template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
    struct pe_serialize<glm::mat<C, R, T, Q>> {
        void operator()(const glm::mat<C, R, T, Q>& m, byte_vector& out) const {
            for (int i = 0; i < R; ++i)
                for (int j = 0; j < C; ++j)
                    core::serialize(m[i][j], out);
        }
    };

    template <glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
    struct pe_deserialize<glm::mat<C, R, T, Q>> {
        void operator()(glm::mat<C, R, T, Q>& m, span<const byte>& in) const {
            for (int i = 0; i < R; ++i)
                for (int j = 0; j < C; ++j)
                    core::deserialize(m[i][j], in);
        }
    };

    template <>
    struct pe_serialize<glm::quat> {
        void operator()(const glm::quat& q, byte_vector& out) const {
            core::serialize_all(out, q.w, q.x, q.y, q.z);
        }
    };

    template <>
    struct pe_deserialize<glm::quat> {
        void operator()(glm::quat& q, span<const byte>& in) const {
            core::deserialize_all(in, q.w, q.x, q.y, q.z);
        }
    };
}

#pragma once

#include <core/helper_macros.hpp>

namespace grx {
    enum class shader_program_id_t : int  {};
    enum class shader_effect_id_t  : int  {};
    enum class uniform_id_t        : int  {};
    enum class texture_id_t        : uint {};

    enum class grx_color_fmt {
        RGB = 0, SRGB, RGB16, RGB16F, RGB32F
    };

    enum class grx_filtering {
        Linear = 0, Nearest
    };

    struct grx_texture {
        uint width = 0, height = 0, channels = 0;
        texture_id_t id = static_cast<texture_id_t>(std::numeric_limits<uint>::max());

        TO_TUPLE_IMPL(width, height, channels, id)
    };

    template <grx_color_fmt ColorFmt, grx_filtering Filtering>
    struct grx_render_target_settings {
        using grx_render_target_settings_check = void;
        static constexpr grx_color_fmt color_fmt = ColorFmt;
        static constexpr grx_filtering filtering = Filtering;
    };

    template <typename T>
    concept RenderTargetSettings = requires { typename T::grx_render_target_settings_check; };
}
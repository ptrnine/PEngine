#pragma once

#include <core/helper_macros.hpp>
#include <core/config_manager.hpp>
#include <core/time.hpp>
#include <type_traits>
#include "grx_shader_mgr.hpp"

namespace grx {
    class grx_postprocess {
    public:
        grx_postprocess(
                const core::config_manager& config_manager,
                core::string_view shader_section_name,
                core::function<void(grx_shader_program&)> program_callback = {}
        ): _program            (grx_shader_program::create_shared(config_manager, shader_section_name)),
           _screen_quad_texture(_program, "screen_quad_texture"),
           program_callback_   (core::move(program_callback))
        {}

        void bind() {
            _program->activate();
            _screen_quad_texture = 0;

            if (program_callback_)
                program_callback_(*_program);
        }

    private:
        core::shared_ptr<grx_shader_program>      _program;
        grx_uniform<int>                          _screen_quad_texture;
        core::function<void(grx_shader_program&)> program_callback_;

    public:
        DECLARE_GET(program)
        DECLARE_NON_CONST_GET(program)

        DECLARE_GET(screen_quad_texture)
        DECLARE_NON_CONST_GET(screen_quad_texture)
    };
} // namespace grx


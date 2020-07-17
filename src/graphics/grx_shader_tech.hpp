#pragma once

#include "grx_shader.hpp"

namespace grx
{
    class grx_shader_tech {
    public:
        grx_shader_tech() = default;
        grx_shader_tech(const core::config_manager& config_manager, core::string_view section);

    private:
        core::shared_ptr<grx_shader_program> _base;
        core::shared_ptr<grx_shader_program> _skeleton;
        core::shared_ptr<grx_shader_program> _instanced;

    public:
        DECLARE_GET(base)
        DECLARE_NON_CONST_GET(base)

        DECLARE_GET(skeleton)
        DECLARE_NON_CONST_GET(skeleton)

        DECLARE_GET(instanced)
        DECLARE_NON_CONST_GET(instanced)
    };

} // namespace grx

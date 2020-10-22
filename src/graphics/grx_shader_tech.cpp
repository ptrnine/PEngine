#include "grx_shader_tech.hpp"

#include <core/config_manager.hpp>

using namespace core;

grx::grx_shader_tech::grx_shader_tech(const config_manager& cm, string_view section) {
    PeRelRequireF(section.starts_with("shader_tech_"),
            "Wrong shader tech section '{}'"
            "Shader tech section must starts with 'shader_tech_' prefix", section);

    auto base_section      = cm.read_unwrap<string>(section, "basic");
    auto skeleton_section  = cm.read_unwrap<string>(section, "skeleton");
    auto instanced_section = cm.read_unwrap<string>(section, "instanced");

    _base      = grx_shader_program::create_shared(cm, base_section);
    _skeleton  = grx_shader_program::create_shared(cm, skeleton_section);
    _instanced = grx_shader_program::create_shared(cm, instanced_section);
}


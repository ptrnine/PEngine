#include "grx_shader_mgr.hpp"
#include <core/container_extensions.hpp>
#include <core/assert.hpp>
#include <core/config_manager.hpp>

#include <GL/glew.h>
#include <GL/glfx.h>

using namespace core;

grx::grx_shader_mgr::~grx_shader_mgr() {
    for (auto& [_, program] : _programs)
        glDeleteProgram(static_cast<GLuint>(program));

    for (auto& [_, effect] : _effects)
        glfxDeleteEffect(static_cast<int>(effect));
}

auto grx::grx_shader_mgr::
compile_program(const string& effect_path, const string& function_name) -> shader_program_id_t {
    Expects(!function_name.empty());
    Expects(!effect_path.empty());

    auto real_effect_path = path_eval(effect_path);

    auto [position, was_inserted] = _effects.emplace(real_effect_path, static_cast<shader_effect_id_t>(-1));

    shader_effect_id_t& effect = position->second;

    if (was_inserted) {
        effect = static_cast<shader_effect_id_t>(glfxGenEffect());
        auto rc = glfxParseEffectFromFile(static_cast<int>(effect), real_effect_path.data());

        RASSERTF(rc, "Error while creating effect from file {}: {}",
                real_effect_path, glfxGetEffectLog(static_cast<int>(effect)));
    }

    auto [program_position, program_was_inserted] =
            _programs.emplace(pair(effect, function_name), static_cast<shader_program_id_t>(-1));

    shader_program_id_t& program = program_position->second;

    if (program_was_inserted) {
        program = static_cast<shader_program_id_t>(
                glfxCompileProgram(static_cast<int>(effect), function_name.data()));

        RASSERTF(static_cast<int>(program) >= 0,
                "Error while compiling program {}:{}: {}",
                real_effect_path, function_name, core::string(glfxGetEffectLog(static_cast<int>(effect))));
    }

    return program;
}

auto grx::grx_shader_mgr::
get_uniform_id(shader_program_id_t program, const core::string& name) -> core::optional<uniform_id_t> {
    auto position = _uniforms.find(pair(name, program));

    if (position == _uniforms.end()) {
        auto gl_uniform = glGetUniformLocation(static_cast<GLuint>(program), name.data());

        RASSERTF(glGetError() == GL_NO_ERROR, "Invalid uniform '{}' location. Shader program not found", name);

        if (gl_uniform == -1)
            return core::nullopt;

        _uniforms.emplace(pair(name, program), static_cast<uniform_id_t>(gl_uniform));

        return static_cast<uniform_id_t>(gl_uniform);
    } else {
        return position->second;
    }
}

auto grx::grx_shader_mgr::
get_uniform_id_unwrap(shader_program_id_t program, const core::string& name) -> uniform_id_t {
    auto uniform = get_uniform_id(program, name);
    if (uniform)
        return *uniform;
    else
        RABORTF("Invalid uniform '{}' location", name);
    return static_cast<uniform_id_t>(-1);
}

auto grx::grx_shader_mgr::
compile_program(const config_manager& cm, string_view section) -> shader_program_id_t {
    RASSERTF(section.starts_with("shader_"),
            "Shader section must starts with 'shared_' prefix "
            "(Actual section: {})", section);

    auto effect_path = cm.entry_dir() /
            cm.read_unwrap<string>("shaders_dir") /
            cm.read_unwrap<string>(section, "effect_path");
    auto entry_func = cm.read_unwrap<string>(section, "entry_function");

    return compile_program(effect_path, entry_func);
}

auto grx::grx_shader_mgr::
load_render_tech(const core::config_manager& cm, core::string_view section) -> grx_shader_tech {
    RASSERTF(section.starts_with("shader_tech_"),
            "Shader tech section must starts with 'shared_tech_' prefix "
            "(Actual section: {})", section);

    auto effect_path = cm.entry_dir() /
            cm.read_unwrap<string>("shaders_dir") /
            cm.read_unwrap<string>(section, "effect_path");

    auto basic     = cm.read_unwrap<string>(section, "basic");
    auto skeleton  = cm.read_unwrap<string>(section, "skeleton");
    auto instanced = cm.read_unwrap<string>(section, "instanced");
    return grx_shader_tech(
            compile_program(effect_path, basic),
            compile_program(effect_path, skeleton),
            compile_program(effect_path, instanced));
}

void grx::grx_shader_mgr::use_program(shader_program_id_t program) {
    glUseProgram(static_cast<uint>(program));
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, int val1) {
    glUniform1i(static_cast<int>(id), val1);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, int val1, int val2) {
    glUniform2i(static_cast<int>(id), val1, val2);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, int val1, int val2, int val3) {
    glUniform3i(static_cast<int>(id), val1, val2, val3);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, int val1, int val2, int val3, int val4) {
    glUniform4i(static_cast<int>(id), val1, val2, val3, val4);
}


void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, uint val1) {
    glUniform1ui(static_cast<int>(id), val1);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, uint val1, uint val2) {
    glUniform2ui(static_cast<int>(id), val1, val2);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, uint val1, uint val2, uint val3) {
    glUniform3ui(static_cast<int>(id), val1, val2, val3);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, uint val1, uint val2, uint val3, uint val4) {
    glUniform4ui(static_cast<int>(id), val1, val2, val3, val4);
}


void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, float val1) {
    glUniform1f(static_cast<int>(id), val1);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, float val1, float val2) {
    glUniform2f(static_cast<int>(id), val1, val2);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, float val1, float val2, float val3) {
    glUniform3f(static_cast<int>(id), val1, val2, val3);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, float val1, float val2, float val3, float val4) {
    glUniform4f(static_cast<int>(id), val1, val2, val3, val4);
}


void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, double val1) {
    glUniform1d(static_cast<int>(id), val1);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, double val1, double val2) {
    glUniform2d(static_cast<int>(id), val1, val2);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, double val1, double val2, double val3) {
    glUniform3d(static_cast<int>(id), val1, val2, val3);
}

void grx::grx_shader_mgr::set_uniform(grx::uniform_id_t id, double val1, double val2, double val3, double val4) {
    glUniform4d(static_cast<int>(id), val1, val2, val3, val4);
}

void grx::grx_shader_mgr::set_uniform(uniform_id_t id, const glm::mat4& matrix) {
    glUniformMatrix4fv(static_cast<int>(id), 1, GL_FALSE, &matrix[0][0]);
}

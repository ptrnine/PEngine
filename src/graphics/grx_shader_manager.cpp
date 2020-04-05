#include "grx_shader_manager.hpp"
#include <core/container_extensions.hpp>
#include <core/assert.hpp>
#include <core/config_manager.hpp>

#include <GL/glew.h>
#include <GL/glfx.h>

using namespace core;

auto grx::grx_shader_manager::
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

auto grx::grx_shader_manager::
get_uniform_id(shader_program_id_t program, const core::string& name) -> uniform_id_t {
    auto [position, was_inserted]
        = _uniforms.emplace(pair(name, program), static_cast<uniform_id_t>(-1));

    uniform_id_t& uniform = position->second;

    if (was_inserted) {
        uniform = static_cast<uniform_id_t>(glGetUniformLocation(static_cast<GLuint>(program), name.data()));

        // For detailed error message
        if (static_cast<int>(uniform) < 0) {
            auto found_program_position =
                std::find_if(_programs.begin(), _programs.end(), [program](auto& pair) {
                    return pair.second == program;
                });

            if (found_program_position == _programs.end()) {
                RABORTF("Invalid uniform '{}' location. Shader program not found", name);
            }
            else {
                auto effect = found_program_position->first.first;
                auto found_effect_position =
                    std::find_if(_effects.begin(), _effects.end(), [effect](auto& pair) {
                        return pair.second == effect;
                    });

                if (found_effect_position == _effects.end())
                    RABORTF("Invalid uniform '{}' location. Shader program entry function is '{}', but"
                            " effect was not found (!?)", name, found_program_position->first.second);
                else
                    RABORTF("Invalid uniform '{}' location in shader {}:{}",
                            name, found_effect_position->first, found_program_position->first.second);
            }
        }
    }

    return uniform;
}

auto grx::grx_shader_manager::
compile_program(const config_manager& cm, string_view section) -> shader_program_id_t {
    RASSERTF(section.starts_with("shader_"), "Shader section must starts with 'shared_' prefix "
                                             "(Actual section: {})", section);

    auto effect_path = cm.entry_dir() /
            cm.read_unwrap<string>("shaders_dir") /
            cm.read_unwrap<string>(section, "effect_path");
    auto entry_func = cm.read_unwrap<string>(section, "entry_function");

    return compile_program(effect_path, entry_func);
}

void grx::grx_shader_manager::use_program(shader_program_id_t program) {
    glUseProgram(static_cast<uint>(program));
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, int val1) {
    glUniform1i(static_cast<int>(id), val1);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, int val1, int val2) {
    glUniform2i(static_cast<int>(id), val1, val2);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, int val1, int val2, int val3) {
    glUniform3i(static_cast<int>(id), val1, val2, val3);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, int val1, int val2, int val3, int val4) {
    glUniform4i(static_cast<int>(id), val1, val2, val3, val4);
}


void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, uint val1) {
    glUniform1ui(static_cast<int>(id), val1);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, uint val1, uint val2) {
    glUniform2ui(static_cast<int>(id), val1, val2);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, uint val1, uint val2, uint val3) {
    glUniform3ui(static_cast<int>(id), val1, val2, val3);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, uint val1, uint val2, uint val3, uint val4) {
    glUniform4ui(static_cast<int>(id), val1, val2, val3, val4);
}


void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, float val1) {
    glUniform1f(static_cast<int>(id), val1);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, float val1, float val2) {
    glUniform2f(static_cast<int>(id), val1, val2);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, float val1, float val2, float val3) {
    glUniform3f(static_cast<int>(id), val1, val2, val3);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, float val1, float val2, float val3, float val4) {
    glUniform4f(static_cast<int>(id), val1, val2, val3, val4);
}


void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, double val1) {
    glUniform1d(static_cast<int>(id), val1);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, double val1, double val2) {
    glUniform2d(static_cast<int>(id), val1, val2);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, double val1, double val2, double val3) {
    glUniform3d(static_cast<int>(id), val1, val2, val3);
}

void grx::grx_shader_manager::set_uniform(grx::uniform_id_t id, double val1, double val2, double val3, double val4) {
    glUniform4d(static_cast<int>(id), val1, val2, val3, val4);
}

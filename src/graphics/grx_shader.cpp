#include "grx_shader.hpp"

#include "grx_config_ext.hpp"
#include "grx_gl_trace.hpp"
#include <core/config_manager.hpp>
#include <core/container_extensions.hpp>

#include <GL/glew.h>

using namespace core;

namespace grx::grx_shader_helper
{
GLenum to_gl_type(shader_type type) {
    switch (type) {
    case shader_type::fragment: return GL_FRAGMENT_SHADER;
    case shader_type::vertex:   return GL_VERTEX_SHADER;
    case shader_type::compute:  return GL_COMPUTE_SHADER;
    }
    ABORT();
    return 0;
}

GLenum to_gl_type(shader_barrier type) {
    switch (type) {
    case shader_barrier::disabled:     return 0;
    case shader_barrier::image_access: return GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
    case shader_barrier::storage:      return GL_SHADER_STORAGE_BARRIER_BIT;
    case shader_barrier::all:          return GL_ALL_BARRIER_BITS;
    }
    ABORT();
    return 0;
}

uint compile(shader_type type, string_view code) {
    auto name = GL_TRACE(glCreateShader, to_gl_type(type));

    auto data   = vector{SHADER_GL_CORE.data(), code.data()};
    auto length = vector{static_cast<int>(SHADER_GL_CORE.size()), static_cast<int>(code.length())};

    if (type == shader_type::compute) {
        data.insert(data.begin() + 1, SHADER_GL_COMPUTE_VARIABLE_GROUP.data());
        length.insert(length.begin() + 1, static_cast<int>(SHADER_GL_COMPUTE_VARIABLE_GROUP.size()));
    }

    GL_TRACE(glShaderSource, name, static_cast<int>(data.size()), data.data(), length.data());
    GL_TRACE(glCompileShader, name);

    int rc = GL_FALSE;
    GL_TRACE(glGetShaderiv, name, GL_COMPILE_STATUS, &rc);

    if (rc == GL_FALSE) {
        int log_length; // NOLINT
        GL_TRACE(glGetShaderiv, name, GL_INFO_LOG_LENGTH, &log_length);

        string msg;
        msg.resize(static_cast<size_t>(log_length));
        GL_TRACE(glGetShaderInfoLog, name, log_length, nullptr, msg.data());

        GL_TRACE(glDeleteShader, name);

        throw shader_exception("GL error: " + msg);
    }

    return name;
}

template <typename StrT>
uint compile_tmpl(shader_type type, const vector<StrT>& code_lines) {
    auto name = GL_TRACE(glCreateShader, to_gl_type(type));

    vector<const char*> lines(code_lines.size() + 1);
    vector<int>         lengths(code_lines.size() + 1);

    lines.front()   = SHADER_GL_CORE.data();
    lengths.front() = static_cast<int>(SHADER_GL_CORE.size());

    int line_start_num = 1;
    if (type == shader_type::compute) {
        lines.push_back(nullptr);
        lines[1] = SHADER_GL_COMPUTE_VARIABLE_GROUP.data();
        lengths.push_back(0);
        lengths[1] = static_cast<int>(SHADER_GL_COMPUTE_VARIABLE_GROUP.size());
        ++line_start_num;
    }

    std::transform(code_lines.begin(), code_lines.end(), lines.begin() + line_start_num, [](auto& s) { return s.data(); });
    std::transform(code_lines.begin(), code_lines.end(), lengths.begin() + line_start_num, [](auto& s) { return s.size(); });

    GL_TRACE(glShaderSource, name, static_cast<GLsizei>(lines.size()), lines.data(), lengths.data());
    GL_TRACE(glCompileShader, name);

    int rc = GL_FALSE;
    GL_TRACE(glGetShaderiv, name, GL_COMPILE_STATUS, &rc);

    if (rc == GL_FALSE) {
        int log_length; // NOLINT
        GL_TRACE(glGetShaderiv, name, GL_INFO_LOG_LENGTH, &log_length);

        string msg;
        msg.resize(static_cast<size_t>(log_length));
        GL_TRACE(glGetShaderInfoLog, name, log_length, nullptr, msg.data());

        GL_TRACE(glDeleteShader, name);

        throw shader_exception("GL error: " + msg);
    }

    return name;
}

template <>
uint compile<string>(shader_type type, const vector<string>& code_lines) {
    return compile_tmpl(type, code_lines);
}

template <>
uint compile<string_view>(shader_type type, const vector<string_view>& code_lines) {
    return compile_tmpl(type, code_lines);
}

void delete_shader(uint name) {
    GL_TRACE(glDeleteShader, name);
}

uint create_program() {
    return GL_TRACE_NO_ARG(glCreateProgram);
}

void attach_shader(uint program, uint shader) {
    GL_TRACE(glAttachShader, program, shader);
}

void link_program(uint program) {
    GL_TRACE(glLinkProgram, program);

    GLint rc = GL_FALSE;
    GL_TRACE(glGetProgramiv, program, GL_LINK_STATUS, &rc);

    if (rc == false) {
        int log_length; // NOLINT
        GL_TRACE(glGetProgramiv, program, GL_INFO_LOG_LENGTH, &log_length);

        string msg;
        msg.resize(static_cast<size_t>(log_length));
        GL_TRACE(glGetProgramInfoLog, program, log_length, nullptr, msg.data());

        GL_TRACE(glDeleteProgram, program);

        throw shader_exception("GL error: " + msg);
    }
}

void delete_program(uint program) {
    GL_TRACE(glDeleteProgram, program);
}

int uniform_location(uint program, const core::string& name) {
    return GL_TRACE(glGetUniformLocation, program, name.data());
}

void activate_program(uint program) {
    GL_TRACE(glUseProgram, program);
}

bool uniform(uint program, int location, float v0) {
    glGetError();
    GL_TRACE(glProgramUniform1f, program, location, v0);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, float v0, float v1) {
    glGetError();
    GL_TRACE(glProgramUniform2f, program, location, v0, v1);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, float v0, float v1, float v2) {
    glGetError();
    GL_TRACE(glProgramUniform3f, program, location, v0, v1, v2);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, float v0, float v1, float v2, float v3) {
    glGetError();
    GL_TRACE(glProgramUniform4f, program, location, v0, v1, v2, v3);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, double v0) {
    glGetError();
    GL_TRACE(glProgramUniform1d, program, location, v0);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, double v0, double v1) {
    glGetError();
    GL_TRACE(glProgramUniform2d, program, location, v0, v1);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, double v0, double v1, double v2) {
    glGetError();
    GL_TRACE(glProgramUniform3d, program, location, v0, v1, v2);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, double v0, double v1, double v2, double v3) {
    glGetError();
    GL_TRACE(glProgramUniform4d, program, location, v0, v1, v2, v3);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, int v0) {
    glGetError();
    GL_TRACE(glProgramUniform1i, program, location, v0);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, int v0, int v1) {
    glGetError();
    GL_TRACE(glProgramUniform2i, program, location, v0, v1);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, int v0, int v1, int v2) {
    glGetError();
    GL_TRACE(glProgramUniform3i, program, location, v0, v1, v2);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, int v0, int v1, int v2, int v3) {
    glGetError();
    GL_TRACE(glProgramUniform4i, program, location, v0, v1, v2, v3);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, uint v0) {
    glGetError();
    GL_TRACE(glProgramUniform1ui, program, location, v0);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, uint v0, uint v1) {
    glGetError();
    GL_TRACE(glProgramUniform2ui, program, location, v0, v1);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, uint v0, uint v1, uint v2) {
    glGetError();
    GL_TRACE(glProgramUniform3ui, program, location, v0, v1, v2);
    return glGetError() == GL_NO_ERROR;
}

bool uniform(uint program, int location, uint v0, uint v1, uint v2, uint v3) {
    glGetError();
    GL_TRACE(glProgramUniform4ui, program, location, v0, v1, v2, v3);
    return glGetError() == GL_NO_ERROR;
}

#define UNIFORM_VEC_FUNC(type, num, glpostfix)                                                                         \
    bool uniform##num(uint program, int location, size_t count, type value) {                                          \
        glGetError();                                                                                                  \
        GL_TRACE(glProgramUniform##num##glpostfix, program, location, static_cast<GLsizei>(count), value);             \
        return glGetError() == GL_NO_ERROR;                                                                            \
    }

#define GEN_UNIFORM_VEC_FUNCS(type, glpostfix)                                                                         \
    UNIFORM_VEC_FUNC(type, 1, glpostfix)                                                                               \
    UNIFORM_VEC_FUNC(type, 2, glpostfix)                                                                               \
    UNIFORM_VEC_FUNC(type, 3, glpostfix)                                                                               \
    UNIFORM_VEC_FUNC(type, 4, glpostfix)

GEN_UNIFORM_VEC_FUNCS(const float*, fv)
GEN_UNIFORM_VEC_FUNCS(const double*, dv)
GEN_UNIFORM_VEC_FUNCS(const int*, iv)
GEN_UNIFORM_VEC_FUNCS(const uint*, uiv)

#define UNIFORM_MAT_FUNC(type, matnum, glmatnum)                                                                       \
    bool uniform##matnum(uint program, int location, size_t count, bool transpose, type value) {                       \
        glGetError();                                                                                                  \
        GL_TRACE(glProgramUniformMatrix##glmatnum,                                                                     \
                 program,                                                                                              \
                 location,                                                                                             \
                 static_cast<GLsizei>(count),                                                                          \
                 static_cast<GLboolean>(transpose),                                                                    \
                 value);                                                                                               \
        return glGetError() == GL_NO_ERROR;                                                                            \
    }

#define GEN_UNIFORM_MAT_FUNCS(type, typepostfix)                                                                       \
    UNIFORM_MAT_FUNC(type, 2x2, 2##typepostfix)                                                                        \
    UNIFORM_MAT_FUNC(type, 2x3, 2x3##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 2x4, 2x4##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 3x2, 3x2##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 3x3, 3##typepostfix)                                                                        \
    UNIFORM_MAT_FUNC(type, 3x4, 3x4##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 4x2, 4x2##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 4x3, 4x3##typepostfix)                                                                      \
    UNIFORM_MAT_FUNC(type, 4x4, 4##typepostfix)

GEN_UNIFORM_MAT_FUNCS(const float*, fv)
GEN_UNIFORM_MAT_FUNCS(const double*, dv)

} // namespace grx::grx_shader_helper

/*
 * Reading shader from configs
 */

template <typename T>
failure_opt<T> read_cfg_value(const pair<const config_manager&, string_view> mgr_section_pair, string_view key) {
    return mgr_section_pair.first.read<T>(mgr_section_pair.second, key);
}

template <typename T>
failure_opt<T> read_cfg_value(const config_section& section, string_view key) {
    return section.read<T>(string(key));
}

string_view get_sectname(const pair<const config_manager&, string_view> mgr_section_pair) {
    return mgr_section_pair.second;
}

const string& get_sectname(const config_section& section) {
    return section.name();
}

enum class cfg_number_type { cfg_float, cfg_double, cfg_int, cfg_uint };

cfg_number_type get_number_type(string_view str) {
    constexpr auto npos = string_view::npos;

    if (str.find('.') != npos || str.find('E') != npos || str.find('e') != npos) {
        if (str.find('f') != npos)
            return cfg_number_type::cfg_float;
        else
            return cfg_number_type::cfg_double;
    }
    else if (str.find('+') != npos || str.find('-') != npos)
        return cfg_number_type::cfg_int;
    else
        return cfg_number_type::cfg_uint;
}

template <typename T>
bool try_pass_to_program(string_view                          program_section,
                         shared_ptr<grx::grx_shader_program>& program,
                         const string&                        key,
                         string_view                          value) {
    try {
        T v = config_cast<T>(value);
        program->get_uniform_unwrap<T>(key).push_unwrap(v);
        return true;
    }
    catch (...) {
        LOG("Pass value to program [{}]: discard value at key '{}'", program_section, key);
        return false;
    }
}

template <size_t N, typename... Ts>
void try_pass_to_program_ts(string_view                          sect,
                            shared_ptr<grx::grx_shader_program>& program,
                            const string&                        key,
                            string_view                          value) {
    if constexpr (N == sizeof...(Ts))
        LOG("Pass value to program [{}]: discard value at key '{}'", sect, key);
    else {
        if (!try_pass_to_program<std::tuple_element_t<N, std::tuple<Ts...>>>(sect, program, key, value))
            try_pass_to_program_ts<N + 1, Ts...>(sect, program, key, value);
    }
}

template <FloatingPoint T>
void try_pass_config_value_to_program_helper(string_view                          sect,
                                             shared_ptr<grx::grx_shader_program>& program,
                                             const string&                        key,
                                             string_view                          value) {
    auto args_count = value / count_if(xlambda(c, c == ',')) + 1;

    switch (args_count) {
    case 1:  try_pass_to_program<      T                                    >(sect, program, key, value); break;
    case 2:  try_pass_to_program<      vec2<T>                              >(sect, program, key, value); break;
    case 3:  try_pass_to_program<      vec3<T>                              >(sect, program, key, value); break;
    case 4:  try_pass_to_program_ts<0, vec4<T>,           glm::mat<2, 2, T> >(sect, program, key, value); break;
    case 6:  try_pass_to_program_ts<0, glm::mat<2, 3, T>, glm::mat<3, 2, T> >(sect, program, key, value); break; // NOLINT
    case 8:  try_pass_to_program_ts<0, glm::mat<2, 4, T>, glm::mat<4, 2, T> >(sect, program, key, value); break; // NOLINT
    case 12: try_pass_to_program_ts<0, glm::mat<3, 4, T>, glm::mat<4, 3, T> >(sect, program, key, value); break; // NOLINT
    case 16: try_pass_to_program<      glm::mat<4, 4, T>                    >(sect, program, key, value); break; // NOLINT
    default: LOG("Pass value to program [{}]: discard value at key '{}'", sect, key);
    }
}

template <Integral T>
void try_pass_config_value_to_program_helper(string_view                          sect,
                                             shared_ptr<grx::grx_shader_program>& program,
                                             const string&                        key,
                                             string_view                          value) {
    auto args_count = value / count_if(xlambda(c, c == ',')) + 1;

    switch (args_count) {
    case 1: try_pass_to_program<T>(sect, program, key, value); break;
    case 2: try_pass_to_program<vec2<T>>(sect, program, key, value); break;
    case 3: try_pass_to_program<vec3<T>>(sect, program, key, value); break;
    case 4: try_pass_to_program<vec4<T>>(sect, program, key, value); break;
    default: LOG("Pass value to program [{}]: discard value at key '{}'", sect, key);
    }
}

void pass_config_value_to_program(string_view                          sect,
                                  shared_ptr<grx::grx_shader_program>& program,
                                  const string&                        key,
                                  string_view                          value) {
    cfg_number_type number_type = get_number_type(value);

    switch (number_type) {
    case cfg_number_type::cfg_int: try_pass_config_value_to_program_helper<int>(sect, program, key, value); break;
    case cfg_number_type::cfg_uint: try_pass_config_value_to_program_helper<uint>(sect, program, key, value); break;
    case cfg_number_type::cfg_float: try_pass_config_value_to_program_helper<float>(sect, program, key, value); break;
    case cfg_number_type::cfg_double: try_pass_config_value_to_program_helper<double>(sect, program, key, value); break;
    default: ABORT();
    }
}

void pass_config_values_to_program(shared_ptr<grx::grx_shader_program>& program,
                                   const config_manager&                cm,
                                   string_view                          section) {
    auto& sect = cm.sections().find(string(section))->second;

    for (auto& [key, _] : sect.values()) {
        if (!grx::is_shader_type_name(key)) {
            auto value = cm.read_unwrap<string>(section, key);
            pass_config_value_to_program(section, program, key, value);
        }
    }
}

void pass_config_values_to_program(shared_ptr<grx::grx_shader_program>& program, const config_section& section) {
    for (auto& [key, _] : section.values()) {
        if (!grx::is_shader_type_name(key)) {
            auto value = section.read_unwrap<string>(key);
            pass_config_value_to_program(section.name(), program, key, value);
        }
    }
}

template <size_t P = 0, typename SectInfoT, grx::Shader... T>
shared_ptr<grx::grx_shader_program>
create_program_iter(const string& shaders_dir, SectInfoT section_info, T&&... shaders) {
    if constexpr (P == grx::shader_type_name_pairs.size()) {
        RASSERTF(sizeof...(shaders), "Shader program section [{}] has no shaders!", get_sectname(section_info));
        return grx::grx_shader_program::create_shared(move(shaders)...);
    }
    else {
        if (auto shader_paths = read_cfg_value<vector<string>>(section_info, grx::shader_type_name_pairs[P].second)) {
            auto shader_codes = *shader_paths / transform<vector<string>>([&](auto& path) {
                return read_file_unwrap(path_eval(shaders_dir / path));
            });

            return create_program_iter<P + 1>(shaders_dir,
                                              section_info,
                                              move(shaders)...,
                                              grx::grx_shader<grx::shader_type_name_pairs[P].first>(shader_codes));
        }
        else
            return create_program_iter<P + 1>(shaders_dir, section_info, move(shaders)...);
    }
}

shared_ptr<grx::grx_shader_program>
grx::grx_shader_program::create_shared(const config_manager& cm, string_view section) {
    if (!section.starts_with("shader_"))
        throw shader_exception("Shader program section name must starts with 'shader_'");

    if (!cm.sections().count(string(section)))
        throw shader_exception("Can't find section [" + string(section) + ']');

    auto shaders_dir = path_eval(cm.entry_dir() / cm.read_unwrap<string>("shaders_dir"));
    auto program     = create_program_iter(shaders_dir, pair<const config_manager&, string_view>(cm, section));

    pass_config_values_to_program(program, cm, section);

    return program;
}

auto grx::grx_shader_program::create_shared(const config_section& section) -> shared_ptr<grx_shader_program> {
    if (!section.name().starts_with("shader_"))
        throw shader_exception("Shader program section name must starts with 'shader_'");

    auto shaders_dir =
        path_eval(DEFAULT_CFG_PATH() / ".." / config_section::direct_read().read_unwrap<string>("shaders_dir"));
    auto program = create_program_iter(shaders_dir, section);

    pass_config_values_to_program(program, section);

    return program;
}

void grx::grx_shader_program::dispatch_compute(
        uint num_groups_x,      uint num_groups_y,      uint num_groups_z,
        uint work_group_size_x, uint work_group_size_y, uint work_group_size_z,
        shader_barrier barrier
) {
    GL_TRACE(glDispatchComputeGroupSizeARB,
            num_groups_x,      num_groups_y,      num_groups_z,
            work_group_size_x, work_group_size_y, work_group_size_z);

    if (barrier != shader_barrier::disabled)
        GL_TRACE(glMemoryBarrier, grx_shader_helper::to_gl_type(barrier));
}


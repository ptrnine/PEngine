#pragma once

#include "grx_types.hpp"
#include <core/assert.hpp>
#include <core/log.hpp>
#include <core/files.hpp>

namespace core
{
class config_manager;
class config_section;
} // namespace core


namespace grx
{
constexpr inline core::string_view SHADER_GL_CORE = "#version 430 core\n";
constexpr inline core::string_view SHADER_GL_COMPUTE_VARIABLE_GROUP = "#extension GL_ARB_compute_variable_group_size: enable\n";

class shader_exception : public std::exception {
public:
    shader_exception(core::string message): msg(std::move(message)) {}

    [[nodiscard]]
    const char* what() const noexcept override {
        return msg.data();
    }

private:
    core::string msg;
};


enum class shader_type    { vertex = 0, fragment, geometry, compute };
enum class shader_barrier { disabled = 0, image_access, storage, all };

inline constexpr auto shader_type_name_pairs =
    core::array{core::pair{shader_type::vertex,   core::string_view("vertex")},
                core::pair{shader_type::fragment, core::string_view("fragment")},
                core::pair{shader_type::geometry, core::string_view("geometry")},
                core::pair{shader_type::compute,  core::string_view("compute")}};

inline bool is_shader_type_name(core::string_view name) {
    return shader_type_name_pairs / core::any_of([name](auto& p) { return p.second == name; });
}

inline core::string_view shader_type_to_string(shader_type type) {
    return shader_type_name_pairs[static_cast<size_t>(type)].second; // NOLINT
}

namespace grx_shader_helper
{
    uint compile(shader_type type, core::string_view code);

    template <typename StrT>
    uint compile(shader_type type, const core::vector<StrT>& code_lines);

    template <>
    uint compile<core::string>(shader_type type, const core::vector<core::string>& code_lines);

    template <>
    uint compile<core::string_view>(shader_type type, const core::vector<core::string_view>& code_lines);

    void delete_shader(uint name);

    uint create_program();
    void attach_shader(uint program, uint shader);
    void link_program(uint program);
    void delete_program(uint program);
    int  uniform_location(uint program, const core::string& name);
    void activate_program(uint program);

    bool uniform(uint program, int location, float v0);
    bool uniform(uint program, int location, float v0, float v1);
    bool uniform(uint program, int location, float v0, float v1, float v2);
    bool uniform(uint program, int location, float v0, float v1, float v2, float v3);

    bool uniform(uint program, int location, double v0);
    bool uniform(uint program, int location, double v0, double v1);
    bool uniform(uint program, int location, double v0, double v1, double v2);
    bool uniform(uint program, int location, double v0, double v1, double v2, double v3);

    bool uniform(uint program, int location, int v0);
    bool uniform(uint program, int location, int v0, int v1);
    bool uniform(uint program, int location, int v0, int v1, int v2);
    bool uniform(uint program, int location, int v0, int v1, int v2, int v3);

    bool uniform(uint program, int location, uint v0);
    bool uniform(uint program, int location, uint v0, uint v1);
    bool uniform(uint program, int location, uint v0, uint v1, uint v2);
    bool uniform(uint program, int location, uint v0, uint v1, uint v2, uint v3);

    bool uniform1(uint program, int location, size_t count, const float* value);
    bool uniform2(uint program, int location, size_t count, const float* value);
    bool uniform3(uint program, int location, size_t count, const float* value);
    bool uniform4(uint program, int location, size_t count, const float* value);

    bool uniform1(uint program, int location, size_t count, const double* value);
    bool uniform2(uint program, int location, size_t count, const double* value);
    bool uniform3(uint program, int location, size_t count, const double* value);
    bool uniform4(uint program, int location, size_t count, const double* value);

    bool uniform1(uint program, int location, size_t count, const int* value);
    bool uniform2(uint program, int location, size_t count, const int* value);
    bool uniform3(uint program, int location, size_t count, const int* value);
    bool uniform4(uint program, int location, size_t count, const int* value);

    bool uniform1(uint program, int location, size_t count, const uint* value);
    bool uniform2(uint program, int location, size_t count, const uint* value);
    bool uniform3(uint program, int location, size_t count, const uint* value);
    bool uniform4(uint program, int location, size_t count, const uint* value);

    bool uniform2x2(uint program, int location, size_t count, bool transpose, const float* value);
    bool uniform2x3(uint program, int location, size_t count, bool transpose, const float* value);
    bool uniform2x4(uint program, int location, size_t count, bool transpose, const float* value);

    bool uniform3x2(uint program, int location, size_t count, bool transpose, const float* value);
    bool uniform3x3(uint program, int location, size_t count, bool transpose, const float* value);
    bool uniform3x4(uint program, int location, size_t count, bool transpose, const float* value);

    bool uniform4x2(uint program, int location, size_t count, bool transpose, const float* value);
    bool uniform4x3(uint program, int location, size_t count, bool transpose, const float* value);
    bool uniform4x4(uint program, int location, size_t count, bool transpose, const float* value);

    bool uniform2x2(uint program, int location, size_t count, bool transpose, const double* value);
    bool uniform2x3(uint program, int location, size_t count, bool transpose, const double* value);
    bool uniform2x4(uint program, int location, size_t count, bool transpose, const double* value);

    bool uniform3x2(uint program, int location, size_t count, bool transpose, const double* value);
    bool uniform3x3(uint program, int location, size_t count, bool transpose, const double* value);
    bool uniform3x4(uint program, int location, size_t count, bool transpose, const double* value);

    bool uniform4x2(uint program, int location, size_t count, bool transpose, const double* value);
    bool uniform4x3(uint program, int location, size_t count, bool transpose, const double* value);
    bool uniform4x4(uint program, int location, size_t count, bool transpose, const double* value);

    template <core::MathVector T>
    bool uniform(uint program, int location, const T& v) {
        static_assert(T::size() >= 2 && T::size() <= 4);

        if constexpr (T::size() == 2)
            return uniform(program, location, v.x(), v.y());
        else if constexpr (T::size() == 3)
            return uniform(program, location, v.x(), v.y(), v.z());
        else if constexpr (T::size() == 4)
            return uniform(program, location, v.x(), v.y(), v.z(), v.w());
    }

    template <core::Number T>
    bool uniform(uint program, int location, core::span<const T> v) {
        return uniform1(program, location, static_cast<size_t>(v.size()), v.data());
    }

    template <core::MathVector T>
    requires(T::size() == 2) bool uniform(uint program, int location, core::span<const T> v) {
        return uniform2(program, location, static_cast<size_t>(v.size()), v.data());
    }

    template <core::MathVector T>
    requires(T::size() == 3) bool uniform(uint program, int location, core::span<const T> v) {
        return uniform3(program, location, static_cast<size_t>(v.size()), v.data());
    }

    template <core::MathVector T>
    requires(T::size() == 4) bool uniform(uint program, int location, core::span<const T> v) {
        return uniform4(program, location, static_cast<size_t>(v.size()), v.data());
    }

    template <glm::length_t C, glm::length_t R, core::FloatingPoint T, glm::qualifier Q>
    bool uniform(uint program, int location, core::span<const glm::mat<C, R, T, Q>> m) {
        if constexpr (C == 2 && R == 2)
            return uniform2x2(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
        else if constexpr (C == 2 && R == 3)
            return uniform2x3(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
        else if constexpr (C == 2 && R == 4)
            return uniform2x4(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
        else if constexpr (C == 3 && R == 2)
            return uniform3x2(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
        else if constexpr (C == 3 && R == 3)
            return uniform3x3(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
        else if constexpr (C == 3 && R == 4)
            return uniform3x4(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
        else if constexpr (C == 4 && R == 2)
            return uniform4x2(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
        else if constexpr (C == 4 && R == 3)
            return uniform4x3(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
        else if constexpr (C == 4 && R == 4)
            return uniform4x4(program, location, static_cast<size_t>(m.size()), false, &(m.at(0)[0][0]));
    }

    template <glm::length_t C, glm::length_t R, core::FloatingPoint T, glm::qualifier Q>
    bool uniform(uint program, int location, const glm::mat<C, R, T, Q>& m) {
        return uniform(program, location, core::span(&m, 1));
    }
} // namespace grx_shader_helper


/**
 * @brief Compiled shader
 *
 * Can't be copied
 *
 * @tparam Type - shader type (vertex, fragment, and etc.)
 */
template <shader_type Type>
class grx_shader {
public:
    using grx_shader_tag = void;

    static constexpr uint no_name = std::numeric_limits<uint>::max();

    static constexpr shader_type type() {
        return Type;
    }

    grx_shader(core::string_view code) {
        _gl_name = grx_shader_helper::compile(Type, code);
    }

    grx_shader(const core::vector<core::string_view>& code_lines) {
        _gl_name = grx_shader_helper::compile(Type, code_lines);
    }

    grx_shader(const core::vector<core::string>& code_lines) {
        _gl_name = grx_shader_helper::compile(Type, code_lines);
    }

    template <typename... Ts> requires((std::is_same_v<Ts, core::string_view> || std::is_same_v<Ts, const char*>)&&...)
    grx_shader(core::string_view line1, core::string_view line2, Ts... lines):
        grx_shader(core::vector<core::string_view>{line1, line2, lines...}) {}

    grx_shader(const grx_shader&) = delete;
    grx_shader& operator=(const grx_shader&) = delete;

    grx_shader(grx_shader&& shader) noexcept: _gl_name(shader._gl_name) {
        shader._gl_name = no_name;
    }

    grx_shader& operator=(grx_shader&& shader) noexcept {
        if (_gl_name != no_name)
            grx_shader_helper::delete_shader(_gl_name);

        _gl_name        = shader._gl_name;
        shader._gl_name = no_name;

        return *this;
    }

    ~grx_shader() {
        grx_shader_helper::delete_shader(_gl_name);
    }

    [[nodiscard]]
    uint raw_id() const {
        return _gl_name;
    }

private:
    uint _gl_name = no_name;
};


template <typename T>
concept Shader = requires {
    typename T::grx_shader_tag;
};


/**
 * @brief Represents a generic shader
 */
class grx_shader_generic {
public:
    struct holder_base { // NOLINT
        virtual ~holder_base() = default;

        [[nodiscard]] virtual uint raw_id() const = 0;
        [[nodiscard]] virtual shader_type type() const = 0;
    };

    template <shader_type Type>
    struct holder : public holder_base {
        explicit holder(grx_shader<Type>&& v) noexcept: val(std::forward<grx_shader<Type>>(v)) {}

        [[nodiscard]] uint raw_id() const override {
            return val.raw_id();
        }

        [[nodiscard]] shader_type type() const override {
            return val.type();
        }

        grx_shader<Type> val;
    };

public:
    template <shader_type Type>
    grx_shader_generic(grx_shader<Type>&& shader): _holder(core::make_unique<holder<Type>>(std::move(shader))) {}

    grx_shader_generic(const grx_shader_generic& shader) noexcept = delete;
    grx_shader_generic& operator=(const grx_shader_generic& shader) noexcept = delete;

    grx_shader_generic(grx_shader_generic&& shader) noexcept = default;
    grx_shader_generic& operator=(grx_shader_generic&& shader) noexcept = default;

    ~grx_shader_generic() = default;

    [[nodiscard]] uint raw_id() const {
        return _holder->raw_id();
    }

    [[nodiscard]] shader_type type() const {
        return _holder->type();
    }

    template <Shader T>
    [[nodiscard]] T* ptr_cast() {
        auto ptr = dynamic_cast<holder<T::type()>*>(_holder.get());
        return ptr ? &ptr->val : nullptr;
    }

    template <Shader T>
    [[nodiscard]] const T* ptr_cast() const {
        auto ptr = dynamic_cast<holder<T::type()>*>(_holder.get());
        return ptr ? &ptr->val : nullptr;
    }

    template <Shader T>
    [[nodiscard]] T& cast() {
        auto res = ptr_cast<T>();
        if (!res)
            throw std::invalid_argument("Invalid grx_shader_generic cast");
        return *res;
    }

    template <Shader T>
    [[nodiscard]] const T& cast() const {
        auto res = ptr_cast<T>();
        if (!res)
            throw std::invalid_argument("Invalid grx_shader_generic cast");
        return *res;
    }

private:
    core::unique_ptr<holder_base> _holder;
};

template <typename T>
concept UniformVal = core::AnyOfType<
    T,
    float,
    int,
    uint,
    double,
    core::vec2f,
    core::vec3f,
    core::vec4f,
    core::vec2d,
    core::vec3d,
    core::vec4d,
    core::vec2i,
    core::vec3i,
    core::vec4i,
    core::vec2u,
    core::vec3u,
    core::vec4u,
    glm::mat2x2,
    glm::mat2x3,
    glm::mat2x4,
    glm::mat3x2,
    glm::mat3x3,
    glm::mat3x4,
    glm::mat4x2,
    glm::mat4x3,
    glm::mat4x4,
    glm::mat<2, 2, double>,
    glm::mat<2, 3, double>,
    glm::mat<2, 4, double>,
    glm::mat<3, 2, double>,
    glm::mat<3, 3, double>,
    glm::mat<3, 4, double>,
    glm::mat<4, 2, double>,
    glm::mat<4, 3, double>,
    glm::mat<4, 4, double>,
    core::span<float>,
    core::span<double>,
    core::span<int>,
    core::span<uint>,
    core::span<core::vec2f>,
    core::span<core::vec3f>,
    core::span<core::vec4f>,
    core::span<core::vec2d>,
    core::span<core::vec3d>,
    core::span<core::vec4d>,
    core::span<core::vec2i>,
    core::span<core::vec3i>,
    core::span<core::vec4i>,
    core::span<core::vec2u>,
    core::span<core::vec3u>,
    core::span<core::vec4u>,
    core::span<glm::mat2x2>,
    core::span<glm::mat2x3>,
    core::span<glm::mat2x4>,
    core::span<glm::mat3x2>,
    core::span<glm::mat3x3>,
    core::span<glm::mat3x4>,
    core::span<glm::mat4x2>,
    core::span<glm::mat4x3>,
    core::span<glm::mat4x4>,
    core::span<glm::mat<2, 2, double>>,
    core::span<glm::mat<2, 3, double>>,
    core::span<glm::mat<2, 4, double>>,
    core::span<glm::mat<3, 2, double>>,
    core::span<glm::mat<3, 3, double>>,
    core::span<glm::mat<3, 4, double>>,
    core::span<glm::mat<4, 2, double>>,
    core::span<glm::mat<4, 3, double>>,
    core::span<glm::mat<4, 4, double>>>;


template <UniformVal T>
class grx_uniform;


/**
 * @brief Represents a compiled shader program
 */
class grx_shader_program : public std::enable_shared_from_this<grx_shader_program> {
public:
    static constexpr uint no_name = std::numeric_limits<uint>::max();

    template <typename... Ts> requires((Shader<Ts>)&&...)
    static core::shared_ptr<grx_shader_program> create_shared(const Ts&... shaders) {
        return core::make_shared<grx_shader_program>(core::constructor_accessor<grx_shader_program>(), shaders...);
    }

    template <typename... Ts> requires((Shader<Ts>)&&...)
    static core::shared_ptr<grx_shader_program> create_shared(
        const core::tuple<Ts...>& shaders) {
        return core::make_shared<grx_shader_program>(core::constructor_accessor<grx_shader_program>(), shaders);
    }

    static core::shared_ptr<grx_shader_program>
    create_shared(const core::config_section& section);

    static core::shared_ptr<grx_shader_program>
    create_shared(const core::config_manager& config_manager, core::string_view section);

    template <typename... Ts> requires((Shader<Ts>)&&...)
    grx_shader_program(core::constructor_accessor<grx_shader_program>::cref, const Ts&... shaders):
        grx_shader_program(shaders...) {}

    template <typename... Ts> requires((Shader<Ts>)&&...)
    grx_shader_program(core::constructor_accessor<grx_shader_program>::cref, const core::tuple<Ts...>& shaders):
        grx_shader_program(shaders) {}

    grx_shader_program(grx_shader_program&& p) noexcept: _gl_name(p._gl_name) {
        p._gl_name = no_name;
    }

    grx_shader_program& operator=(grx_shader_program&& p) noexcept {
        _gl_name   = p._gl_name;
        p._gl_name = no_name;
        return *this;
    }

    grx_shader_program(const grx_shader_program&) = delete;
    grx_shader_program& operator=(const grx_shader_program&) = delete;

    ~grx_shader_program() {
        if (_gl_name != no_name)
            grx_shader_helper::delete_program(_gl_name);
    }

    uint raw_id() const {
        return _gl_name;
    }

    /**
     * @brief Gets uniform
     *
     * @throw shader_exception if uniform no found
     *
     * @tparam U - the type of the uniform value
     * @param name - name of the uniform
     *
     * @return the uniform
     */
    template <UniformVal U>
    grx_uniform<U> get_uniform_unwrap(const core::string& name);

    /**
     * @brief Gets uniform
     *
     * @tparam U - the type of the uniform value
     * @param name - name of the uniform
     *
     * @return the uniform or exception in try_opt
     */
    template <UniformVal U>
    core::try_opt<grx_uniform<U>> get_uniform(const core::string& name) {
        try {
            return get_uniform_unwrap<U>(name);
        }
        catch (const shader_exception& e) {
            return shader_exception(e.what());
        }
    }

    /**
     * @brief Activate shader program
     */
    void activate() const {
        grx_shader_helper::activate_program(_gl_name);
    }

    /**
     * @brief Run compute shader
     */
    void dispatch_compute(uint num_groups_x,      uint num_groups_y,      uint num_groups_z,
                          uint work_group_size_x, uint work_group_size_y, uint work_group_size_z,
                          shader_barrier barrier = shader_barrier::disabled);

private:
    template <typename... Ts, size_t... Idxs>
    static void _init(uint program, const core::tuple<Ts...>& shaders, std::index_sequence<Idxs...>&&) {
        (grx_shader_helper::attach_shader(program, std::get<Idxs>(shaders).raw_id()), ...);
    }

    template <typename... Ts> requires((Shader<Ts>)&&...)
    grx_shader_program(const Ts&... shaders) {
        PeRelRequireF(sizeof...(shaders), "{}", "Attempt to create shader program without shaders");

        _gl_name = grx_shader_helper::create_program();
        (grx_shader_helper::attach_shader(_gl_name, shaders.raw_id()), ...);
        grx_shader_helper::link_program(_gl_name);
    }

    template <typename... Ts> requires((Shader<Ts>)&&...)
    grx_shader_program(const core::tuple<Ts...>& shaders) {
        static_assert(sizeof...(Ts) > 0, "Attempt to create shader program without shaders");
        _gl_name = grx_shader_helper::create_program();
        _init(_gl_name, shaders);
        grx_shader_helper::link_program(_gl_name);
    }

    uint _gl_name = no_name;
};


template <typename T>
struct grx_add_const_to_span_element_type__ {
    using type = T;
};

template <typename T>
struct grx_add_const_to_span_element_type__<core::span<T>> {
    using type = core::span<const T>;
};

/**
 * @brief Represents a shader uniform
 *
 * @tparam T - the type of uniform value
 */
template <UniformVal T>
class grx_uniform {
public:
    using element_type = typename grx_add_const_to_span_element_type__<T>::type;

    grx_uniform() = default;

    grx_uniform(core::weak_ptr<grx_shader_program> parent_program, core::string name):
        _parent(core::move(parent_program)), _name(std::move(name)) {
        if (auto program = _parent.lock()) {
            _gl_loc = grx_shader_helper::uniform_location(program->raw_id(), _name);
            if (_gl_loc < 0)
                throw shader_exception("Invalid location of uniform \"" + _name + '"');
        }
        else {
            throw shader_exception("Parent program was destroyed");
        }
    }

    /**
     * @brief Push a value to the uniform
     *
     * @throw shader_exception if value can't be pushed
     *
     * @param value - the value to be push
     *
     * @return *this
     */
    //template <typename TT>
    //auto& operator=(const TT& value) {
    //    push_unwrap(T(value));
    //    return *this;
    //}

    /**
     * @brief Push a value to the uniform
     *
     * @throw shader_exception if value can't be pushed
     *
     * @param value - the value to be push
     *
     * @return *this
     */
    auto& operator=(const element_type& value) const {
        push_unwrap(value);
        return *this;
    }

    /**
     * @brief Push a value to the uniform
     *
     * @param value - the value to be pushed
     *
     * @return true if successful, false otherwise
     */
    [[nodiscard]]
    bool push(const element_type& value) const {
        if (auto program = _parent.lock())
            return grx_shader_helper::uniform(program->raw_id(), _gl_loc, value);
        else
            return false;
    }

    /**
     * @brief Push a value to the uniform
     *
     * @throw shader_exception if value can't be pushed
     *
     * @param value - the value to be pushed
     */
    void push_unwrap(const element_type& value) const {
        if (auto program = _parent.lock()) {
            if (!grx_shader_helper::uniform(program->raw_id(), _gl_loc, value))
                throw shader_exception("Can't push a value to the uniform \"" + _name + '"');
        }
        else
            throw shader_exception("Can't push a value to the uniform \"" + _name + "\": parent program was destroyed");
    }

private:
    friend class grx_shader_program;
    core::weak_ptr<grx_shader_program> _parent;
    core::string                       _name;
    int                                _gl_loc = -1;
};

template <UniformVal U>
grx_uniform<U> grx_shader_program::get_uniform_unwrap(const core::string& name) {
    return grx_uniform<U>(weak_from_this(), name);
}

/**
 * @brief Loads shader from file
 *
 * @tparam T - the type of new shader
 * @param shader_path - the path to shader file
 *
 * @return created shader or exception_ptr in try_opt
 */
template <shader_type T>
core::try_opt<grx_shader<T>> load_shader(const core::string& shader_path) {
    auto file = core::read_file(shader_path);

    if (file) {
        try {
            return grx_shader<T>(*file);
        }
        catch (const shader_exception& e) {
            return shader_exception(core::format("Can't load shader at path {}: {}", shader_path, e.what()));
        }
    }

    return std::runtime_error(core::format("Can't open shader file '{}'", shader_path));
}

/**
 * @brief Loads shader from file
 *
 * @throw exception if loading failed
 *
 * @tparam T - the type of new shader
 * @param shader_path - the path to shader file
 *
 * @return created shader
 */
template <shader_type T>
grx_shader<T> load_shader_unwrap(const core::string& shader_path) {
    auto file = core::read_file_unwrap(shader_path);
    return grx_shader<T>(file);
}

} // namespace grx

#pragma once

#include "grx_shader.hpp"

namespace grx {

namespace details {
    template <typename T>
    struct uniform_type { using type = grx_uniform<T>; };

    template <typename T>
    struct uniform_type<core::optional<T>> { using type = core::optional<grx_uniform<T>>; };
}

template <typename T>
struct grx_uniform_tag {
    using value_type = T;
    grx_uniform_tag(core::string uniform_name): name(core::move(uniform_name)) {}
    template <size_t S>
    grx_uniform_tag(const char(&uniform_name)[S]): name(uniform_name, S) {}
    core::string name;
};

template <typename... Ts>
class grx_uniform_cache {
public:
    grx_uniform_cache(grx_uniform_tag<Ts>... names): _names{names.name...} {}

    void setup(const core::shared_ptr<grx_shader_program>& shader_program) {
        if (_program != shader_program.get()) {
            _program = shader_program.get();
            setup(std::make_index_sequence<sizeof...(Ts)>());
        }
    }

    template <size_t Idx>
    const auto& get() const {
        using std::get;
        return get<Idx>(_uniforms);
    }

    template <size_t Idx>
    auto& get() {
        using std::get;
        return get<Idx>(_uniforms);
    }

private:
    template <size_t... Idxs>
    void setup(std::index_sequence<Idxs...>&&) {
        using std::get;
        _uniforms = core::tuple{get_uniform<Ts>(get<Idxs>(_names))...};
    }

    template <typename T>
    auto get_uniform(const core::string& name) {
        if constexpr (core::Optional<T>)
            return _program->try_get_uniform<typename T::value_type>(name).to_optional();
        else
            return _program->get_uniform<T>(name);
    }

private:
    grx_shader_program*                                      _program = nullptr;
    core::tuple<typename details::uniform_type<Ts>::type...> _uniforms;
    core::array<core::string, sizeof...(Ts)>                 _names;
};
}

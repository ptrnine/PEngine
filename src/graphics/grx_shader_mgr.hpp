#pragma once

#include "grx_shader.hpp"
#include "grx_types.hpp"
#include <core/md5.hpp>
#include <core/vec.hpp>

namespace core
{
class config_manager;
}

namespace grx
{
namespace shader_helper
{
    template <shader_type T>
    using strview_eval_t = core::string_view;
}

class grx_shader_program_mgr : public std::enable_shared_from_this<grx_shader_program_mgr> {
public:
    template <shader_type... Types>
    core::shared_ptr<grx_shader_program> load(shader_helper::strview_eval_t<Types>... paths) {
        static_assert(sizeof...(Types) > 0);

        core::string key              = ((core::path_eval(paths) + ";") + ...);
        auto [position, was_inserted] = _program_map.emplace(key, nullptr);

        if (was_inserted)
            position->second = grx_shader_program::create_shared(load_shader<Types>(core::path_eval(paths))...);

        return position->second;
    }

    core::shared_ptr<grx_shader_program> load(const core::config_manager& config_manager, core::string_view section) {
        /*
         * Now we use shader section name as key.
         * That not compatible with constructor from paths
         * i.e shader program constructed from identical shaders paths wouldn't be replaced.
         *
         * I think it's normal way, because shader program read from config may contain uniforms and
         * therefore can't replace shader program (or can't be replaced by the shader program) from plain files
         */
        auto [position, was_inserted] = _program_map.emplace(core::string(section), nullptr);

        if (was_inserted)
            position->second = grx_shader_program::create_shared(config_manager, section);

        return position->second;
    }

private:
    core::hash_map<core::string, core::shared_ptr<grx_shader_program>> _program_map;
};
} // namespace grx

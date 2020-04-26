#pragma once

#include <core/helper_macros.hpp>
#include <core/config_manager.hpp>
#include "grx_shader_mgr.hpp"

namespace grx {

    class grx_postprocess {
    public:
        using uniform_vector_t   = core::vector<uniform_id_t>;
        using uniform_callback_t = std::function<void(const uniform_vector_t&)>;

        grx_postprocess(
                grx_shader_mgr& shader_manager,
                core::config_manager& config_manager,
                core::string_view shader_section_name
        ) {
            _program_id          = shader_manager.compile_program(config_manager, shader_section_name);
            _screen_quad_texture = shader_manager.get_uniform_id_unwrap(_program_id, "screen_quad_texture");
        }

        grx_postprocess(
                grx_shader_mgr& shader_manager,
                core::config_manager& config_manager,
                core::string_view shader_section_name,
                const core::vector<core::string>& uniform_names,
                uniform_callback_t uniform_callback
        ): _uniform_callback(std::move(uniform_callback))
        {
            _program_id          = shader_manager.compile_program(config_manager, shader_section_name);
            _screen_quad_texture = shader_manager.get_uniform_id_unwrap(_program_id, "screen_quad_texture");

            _uniforms.resize(uniform_names.size());
            for (size_t i = 0; i < uniform_names.size(); ++i)
                _uniforms[i] = shader_manager.get_uniform_id_unwrap(_program_id, uniform_names[i]);
        }

        void bind_program_and_uniforms() const {
            grx::grx_shader_mgr::use_program(_program_id);
            grx::grx_shader_mgr::set_uniform(_screen_quad_texture, 0); // Texture position

            if (_uniform_callback)
                _uniform_callback(_uniforms);
        }


    private:
        shader_program_id_t _program_id;
        uniform_id_t        _screen_quad_texture;
        uniform_vector_t    _uniforms;
        uniform_callback_t  _uniform_callback;

    protected:
        template <typename> friend class grx_postprocess_mgr;
        DECLARE_VAL_GET(program_id)
        DECLARE_VAL_GET(screen_quad_texture)
    };

    inline grx_postprocess::uniform_callback_t postprocess_uniform_seconds() {
        return [](const grx::grx_postprocess::uniform_vector_t& it) {
            grx::grx_shader_mgr::set_uniform(it[0],
                                             static_cast<float>(core::global_timer().measure<core::microseconds>() / 1000000.0));
        };
    }
} // namespace grx

#define POSTPROCESS_UNIFORM_CALLBACK(UNIFORMS_NAME) [](const grx::grx_postprocess::uniform_vector_t& UNIFORMS_NAME)

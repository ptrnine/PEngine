#pragma once

#include "grx_types.hpp"
#include <core/vec.hpp>

namespace core {
    class config_manager;
    class config_section;
}

namespace std {
    template <>
    struct hash<pair<string, grx::shader_program_id_t>> {
        inline size_t operator()(const pair <string, grx::shader_program_id_t>& p) const {
            return std::hash<string>()(p.first) ^ std::hash<grx::shader_program_id_t>()(p.second);
        }
    };

    template <>
    struct hash<pair<grx::shader_effect_id_t, string>> {
        inline size_t operator()(const pair<grx::shader_effect_id_t, string>& p) const {
            return std::hash<grx::shader_effect_id_t>()(p.first) ^ std::hash<string>()(p.second);
        }
    };
}

namespace grx {
    class grx_shader_tech {
    private:
        friend class grx_shader_mgr;
        grx_shader_tech(shader_program_id_t base, shader_program_id_t skeleton, shader_program_id_t instanced):
            _base(base), _skeleton(skeleton), _instanced(instanced) {}

        shader_program_id_t _base;
        shader_program_id_t _skeleton;
        shader_program_id_t _instanced;

    public:
        DECLARE_VAL_GET(base)
        DECLARE_VAL_GET(skeleton)
        DECLARE_VAL_GET(instanced)
    };


    class grx_shader_compiler {
    public:
        shader_program_id_t compile(const core::string& effect);
    };


    class grx_shader_mgr {
    public:
        ~grx_shader_mgr();

        shader_program_id_t compile_program(
                const core::string& effect_path,
                const core::string& function_name);

        shader_program_id_t compile_program(
                const core::config_manager& cm,
                core::string_view shader_section_name);

        grx_shader_tech load_render_tech(
                const core::config_manager& cm,
                core::string_view section);

        shader_program_id_t compile_program(
                const core::config_section& cm,
                core::string_view shader_section_name);

        grx_shader_tech load_render_tech(
                const core::config_section& cm,
                core::string_view section);


        core::optional<uniform_id_t>
        get_uniform_id(shader_program_id_t program, const core::string& name);

        uniform_id_t
        get_uniform_id_unwrap(shader_program_id_t program, const core::string& name);

        static void set_uniform(uniform_id_t id, int val1);
        static void set_uniform(uniform_id_t id, int val1, int val2);
        static void set_uniform(uniform_id_t id, int val1, int val2, int val3);
        static void set_uniform(uniform_id_t id, int val1, int val2, int val3, int val4);

        static void set_uniform(uniform_id_t id, uint val1);
        static void set_uniform(uniform_id_t id, uint val1, uint val2);
        static void set_uniform(uniform_id_t id, uint val1, uint val2, uint val3);
        static void set_uniform(uniform_id_t id, uint val1, uint val2, uint val3, uint val4);

        static void set_uniform(uniform_id_t id, float val1);
        static void set_uniform(uniform_id_t id, float val1, float val2);
        static void set_uniform(uniform_id_t id, float val1, float val2, float val3);
        static void set_uniform(uniform_id_t id, float val1, float val2, float val3, float val4);

        static void set_uniform(uniform_id_t id, double val1);
        static void set_uniform(uniform_id_t id, double val1, double val2);
        static void set_uniform(uniform_id_t id, double val1, double val2, double val3);
        static void set_uniform(uniform_id_t id, double val1, double val2, double val3, double val4);

        static void set_uniform(uniform_id_t id, const glm::mat4& matrix);

        template <typename T>
        static void set_uniform(uniform_id_t id, const core::vec<T, 2>& vec) {
            set_uniform(id, vec.x(), vec.y());
        }

        template <typename T>
        static void set_uniform(uniform_id_t id, const core::vec<T, 3>& vec) {
            set_uniform(id, vec.x(), vec.y(), vec.z());
        }

        template <typename T>
        static void set_uniform(uniform_id_t id, const core::vec<T, 4>& vec) {
            set_uniform(id, vec.x(), vec.y(), vec.z(), vec.w());
        }

        template <typename T, typename... Ts>
        void set_uniform(shader_program_id_t id, const core::string& name, T val, Ts... values) {
            set_uniform(get_uniform_id_unwrap(id, name), val, values...);
        }

        static void use_program(shader_program_id_t program);

    private:
        core::hash_map<core::pair<core::string, shader_program_id_t>, uniform_id_t>       _uniforms;
        core::hash_map<core::pair<shader_effect_id_t, core::string>, shader_program_id_t> _programs;
        core::hash_map<core::string, shader_effect_id_t>                                  _effects;
    };
} // namespace grx

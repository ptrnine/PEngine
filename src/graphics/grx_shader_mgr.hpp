#pragma once
#include <core/types.hpp>
#include <core/vec.hpp>
#include "grx_types.hpp"

namespace core {
    class config_manager;
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
    class grx_shader_mgr {
    public:
        shader_program_id_t compile_program(
                const core::string& effect_path,
                const core::string& function_name);

        shader_program_id_t compile_program(
                const core::config_manager& cm,
                core::string_view shader_section_name);

        uniform_id_t get_uniform_id(shader_program_id_t program, const std::string& name);

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
            set_uniform(get_uniform_id(id, name), val, values...);
        }

        static void use_program(shader_program_id_t program);

    private:
        core::hash_map<core::pair<core::string, shader_program_id_t>, uniform_id_t>       _uniforms;
        core::hash_map<core::pair<shader_effect_id_t, core::string>, shader_program_id_t> _programs;
        core::hash_map<core::string, shader_effect_id_t>                                  _effects;
    };
} // namespace grx

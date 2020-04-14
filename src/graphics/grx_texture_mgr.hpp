#pragma once

#include <core/config_manager.hpp>
#include "grx_types.hpp"

namespace grx {
    class grx_texture_mgr {
    public:
        grx_texture load(core::string_view path);
        grx_texture load(const core::config_manager& config_mgr, core::string_view path);

    private:
        core::hash_map<core::string, grx_texture> _texture_ids;
    };

} // namespace grx
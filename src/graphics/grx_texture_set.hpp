#pragma once

#include "core/async.hpp"
#include "grx_texture_mgr.hpp"
#include "grx_texture_path_set.hpp"

namespace grx {

template <size_t S>
class grx_texture_set
    : public core::option_array<grx_texture_id<S>,
                                grx_texture_set_tag,
                                static_cast<size_t>(
                                    grx_texture_set_tag::grx_texture_set_tag_count)> {
public:
    void serialize(core::vector<core::byte>& s) const {
        core::serialize(this->options(), s);
    }

    void deserialize(core::span<const core::byte>& d) {
        core::deserialize(this->options(), d);
    }

    grx_texture_set() = default;

    grx_texture_set(const core::shared_ptr<grx_texture_mgr<S>>& texture_mgr,
                    const grx_texture_path_set&                 texture_paths) {
        using core::operator/;

        for (auto& [path, i] : core::value_index_view(texture_paths.options())) {
            auto tag = static_cast<grx_texture_set_tag>(i);
            if (path)
                this->set(tag, texture_mgr->load(*path));
            else {
                if (tag == grx_texture_set_tag::diffuse)
                    this->set(tag, texture_mgr->load(core::cfg_path("textures_dir", "basic/dummy_diffuse.png")));
                else if (tag == grx_texture_set_tag::normal)
                    this->set(tag, texture_mgr->load(core::cfg_path("textures_dir", "basic/dummy_normal.png")));
            }
        }
    }
};

template <size_t S>
core::vector<grx_texture_set<S>>
load_texture_set_from_paths(const core::shared_ptr<grx_texture_mgr<S>>& texture_mgr,
                            const core::vector<grx_texture_path_set>&   paths) {
    core::vector<grx_texture_set<S>> textures;
    for (auto& path_set : paths)
        textures.emplace_back(grx_texture_set<S>(texture_mgr, path_set));

    return textures;
}

} // namespace grx


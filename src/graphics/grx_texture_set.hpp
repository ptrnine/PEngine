#pragma once

#include "core/async.hpp"
#include "core/resource_mgr_base.hpp"
#include "grx_texture_mgr.hpp"
#include "grx_texture_path_set.hpp"

namespace grx {

template <ColorComponent T, size_t S>
class grx_texture_set
    : public core::option_array<grx_texture_provider<T, S>,
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

    grx_texture_set(const core::shared_ptr<grx_texture_mgr<T, S>>& texture_mgr,
                    const grx_texture_path_set&                    texture_paths) {
        for (auto& [path, i] : core::value_index_view(texture_paths.options())) {
            auto tag = static_cast<grx_texture_set_tag>(i);

            if (path)
                this->set(tag, texture_mgr->load(path->path, path->load_significance));
        }
    }

    grx_texture_set(const grx_texture_path_set& texture_paths) {
        for (auto& [path, i] : core::value_index_view(texture_paths.options())) {
            auto tag = static_cast<grx_texture_set_tag>(i);

            if (path) {
                auto mgr = core::mgr_lookup<grx_texture_mgr<T, S>>(path->mgr_tag);
                this->set(tag, mgr->load(path->path, path->load_significance));
            }
        }
    }

    [[nodiscard]]
    grx_texture_path_set to_path_set() const {
        grx_texture_path_set result;
        for (auto& [texture_provider, i] : core::value_index_view(this->options())) {
            auto tag = static_cast<grx_texture_set_tag>(i);
            if (texture_provider)
                result.set(tag, texture_provider->to_resource_path());
        }
        return result;
    }
};

template <ColorComponent T, size_t S>
core::vector<grx_texture_set<T, S>>
load_texture_set_from_paths(const core::shared_ptr<grx_texture_mgr<T, S>>& texture_mgr,
                            const core::vector<grx_texture_path_set>&      paths) {
    core::vector<grx_texture_set<T, S>> textures;
    for (auto& path_set : paths)
        textures.emplace_back(grx_texture_set<T, S>(texture_mgr, path_set));

    return textures;
}

template <ColorComponent T, size_t S>
core::vector<grx_texture_set<T, S>>
load_texture_sets_from_paths(const core::vector<grx_texture_path_set>& paths) {
    core::vector<grx_texture_set<T, S>> textures;
    for (auto& path_set : paths)
        textures.emplace_back(grx_texture_set<T, S>(path_set));

    return textures;
}

template <ColorComponent T, size_t S>
core::vector<grx_texture_path_set> texture_sets_to_paths(const core::vector<grx_texture_set<T, S>>& texture_sets) {
    core::vector<grx_texture_path_set> texture_paths;
    for (auto& texture_provider : texture_sets)
        texture_paths.emplace_back(texture_provider.to_path_set());

    return texture_paths;
}

} // namespace grx


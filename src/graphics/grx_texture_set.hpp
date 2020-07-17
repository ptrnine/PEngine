#include "core/async.hpp"
#include "grx_texture_mgr.hpp"

namespace grx {

template <size_t S>
class grx_texture_set {
public:
    enum type { diffuse = 0, normal, specular, types_count };

    using deffered_t = core::deffered_resource<grx_texture_id_future<S>>;

    grx_texture_set() = default;

    template <typename... Ts>
    grx_texture_set(Ts&&... texture_id_futures): textures_(core::forward<Ts>(texture_id_futures)...) {}

    [[nodiscard]]
    deffered_t& get_resource(size_t index) {
        return textures_.at(index);
    }

    [[nodiscard]]
    const deffered_t& get_resource(size_t index) const {
        return textures_.at(index);
    }

    [[nodiscard]]
    core::failure_opt<grx_texture_id<S>>& get(size_t index) {
        return textures_.at(index).get();
    }

    [[nodiscard]]
    const core::failure_opt<grx_texture_id<S>>& get(size_t index) const {
        return textures_.at(index).get();
    }

    [[nodiscard]]
    grx_texture_id<S>& get_unwrap(size_t index) {
        return textures_.at(index).get_unwrap();
    }

    [[nodiscard]]
    const grx_texture_id<S>& get_unwrap(size_t index) const {
        return textures_.at(index).get_unwrap();
    }

    void set(size_t index, grx_texture_id_future<S>&& f) {
        if (index >= textures_.size())
            textures_.resize(index + 1);
        textures_[index] = deffered_t(std::move(f));
    }

    void set(size_t index) {
        if (index >= textures_.size())
            textures_.resize(index + 1);
    }

private:
    core::vector<deffered_t> textures_;
};



} // namespace grx


#pragma once

#include "../range.hpp"

namespace core::acts
{
template <typename F>
struct drop_when {
    drop_when(F binary_op): _binary_op(move(binary_op)) {}

    template <InputRange Rng>
    auto operator()(Rng&& rng) {
        auto b = begin(rng);
        auto e = end(rng);
        auto nb = begin(rng);

        if constexpr (detail::RangeFunc<F, decltype(nb)>) {
            while (nb != e && !_binary_op(nb, e))
                ++nb;
        } else {
            while (nb != e && !_binary_op(*nb))
                ++nb;
        }
        return range(b, nb);
    }

    friend auto operator/(InputRange auto&& rng, drop_when dr) {
        return dr(rng);
    }

    F _binary_op;
};
} // namespace core::acts

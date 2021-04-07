#pragma once

#include "../range.hpp"

namespace core::acts
{
template <typename PrefT>
struct drop_pref {
    drop_pref(PrefT prefix): _pref(move(prefix)) {}

    template <typename I, typename P = PrefT>
    void drop_pref_op(I& b, I& e) const {
        if (b == e)
            return;

        if constexpr (Range<P>) {
            auto pref_b = begin(_pref);
            auto pref_e = end(_pref);
            auto tmp_b  = b;

            for (; pref_b != pref_e && tmp_b != e; ++pref_b, ++tmp_b) {
                if (*pref_b != *tmp_b)
                    break;
            }

            if (pref_b == pref_e)
                b = tmp_b;
        }
        else {
            if (*b == _pref)
                ++b;
        }
    }

    template <BiderectRange Rng>
    auto operator()(Rng&& rng) const {
        auto b = begin(rng);
        auto e = end(rng);
        drop_pref_op(b, e);
        return range{b, e};
    }

    friend auto operator/(BiderectRange auto&& rng, drop_pref drop_pref) {
        return drop_pref(rng);
    }

    PrefT _pref;
};
} // namespace core::acts

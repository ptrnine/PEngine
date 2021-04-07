#pragma once

#include "../range.hpp"

namespace core::acts
{
template <typename C>
struct unbracket {
    unbracket(C open_bracket, C close_bracket): _o(move(open_bracket)), _c(move(close_bracket)) {}

    template <BiderectRange Rng>
    auto operator()(Rng&& rng) {
        auto b = begin(rng);
        auto e = end(rng);

        constexpr auto check = [](const auto& b, const auto& e, const auto& o, const auto& c) {
            bool open_satisfy = false;
            if (b != e && *b == o && std::next(b) != e)
                open_satisfy = true;
            if (open_satisfy && *std::prev(e) == c)
                return true;
            return false;
        };

        if constexpr (Range<C>) {
            for (auto open_i = begin(_o), close_i = begin(_c); open_i != end(_o) && close_i != end(_c); ++open_i, ++close_i) {
                if (check(b, e, *open_i, *close_i))
                    return range(std::next(b), std::prev(e));
            }
        } else {
            if (check(b, e, _o, _c))
                return range(std::next(b), std::prev(e));
        }
        return range(b, e);
    }

    friend auto operator/(BiderectRange auto&& rng, unbracket un) {
        return un(rng);
    }

    C _o, _c;
};
} // namespace core::acts


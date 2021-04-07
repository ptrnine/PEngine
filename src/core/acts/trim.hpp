#pragma once

#include "../range.hpp"

namespace core::acts
{

template <typename F>
struct trim_when {
    trim_when(F ibinary_op): binary_op(move(ibinary_op)) {}

    void trim_op(auto& b, auto& e) {
        while (b != e && binary_op(*b)) ++b;
        while (e != b && binary_op(*std::prev(e))) --e;
    }

    template <BiderectRange Rng>
    auto operator()(Rng&& rng) {
        auto b = begin(rng);
        auto e = end(rng);
        trim_op(b, e);
        return range(b, e);
    }

    friend auto operator/(BiderectRange auto&& rng, trim_when tr) {
        return tr(rng);
    }

    F binary_op;
};

template <typename C, size_t N>
struct trim {
    template <typename ArgT, typename... ArgsT>
    trim(ArgT trimmed_symbol, ArgsT... trimmed_symbols):
        cs{move(trimmed_symbol), move(trimmed_symbols)...} {}

    void trim_op(auto& b, auto& e) const {
        constexpr auto has = [](const array<C, N>& cs, auto cur) {
            bool result = false;
            for (auto c : cs) result = result || c == cur;
            return result;
        };
        while (b != e && has(cs, *b)) ++b;
        while (b != e && has(cs, *std::prev(e))) --e;
    }

    template <BiderectRange Rng>
    auto operator()(Rng&& rng) const {
        auto b = begin(rng);
        auto e = end(rng);
        trim_op(b, e);
        return range(b, e);
    }

    friend auto operator/(BiderectRange auto&& rng, trim tr) {
        return tr(rng);
    }

    array<C, N> cs;
};

template <typename C, typename... Cs>
trim(C, Cs...) -> trim<C, sizeof...(Cs) + 1>;
} // namespace core::acts

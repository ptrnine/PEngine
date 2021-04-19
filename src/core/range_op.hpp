#pragma once

#include "range.hpp"

namespace core
{
template <typename F>
constexpr inline auto oneline_comment_start(F disabler) {
    return [disabler]<typename I>(const I& b, const I& e) mutable {
        auto c = *b;
        bool disable;
        if constexpr (detail::RangeFunc<F, I>)
            disable = disabler(b, e);
        else
            disable = disabler(c);

        if (!disable) {
            if (c == ';')
                return true;
            if (c == '/') {
                auto n = std::next(b);
                if (n != e) {
                    auto nc = *n;
                    if (nc == '/')
                        return true;
                }
            }
        }
        return false;
    };
}

constexpr inline auto oneline_comment_start() {
    return oneline_comment_start([](auto) { return false; });
}

template <typename T>
auto subrange_excluder(T open_tk, T close_tk) {
    constexpr auto equal_skip = [](auto& b, auto e, auto tk_b, auto tk_e) {
        auto nb = b;
        auto prev_nb = nb;
        while (nb != e && tk_b != tk_e && *nb == *tk_b) {
            prev_nb = nb;
            ++nb;
            ++tk_b;
        }
        if (tk_b == tk_e) {
            b = prev_nb;
            return true;
        }
        return false;
    };

    return [equal_skip, open_tk, close_tk, on_sub = false](auto& b, auto e) mutable {
        if (on_sub) {
            if (equal_skip(b, e, begin(close_tk), end(close_tk)))
                on_sub = false;
            return true;
        } else {
            if (equal_skip(b, e, begin(open_tk), end(open_tk))) {
                on_sub = true;
                return true;
            }
            return false;
        }
    };
}

constexpr inline auto exclude_in_quotes() {
    return [state = 0](auto c) mutable {
        switch (state) {
        default:
            switch (c) {
            case '\'':
                state = 1;
                return true;
            case '"':
                state = 2;
                return true;
            default: break;
            }
            return false;
        case 1:
            if (c == '\'')
                state = 0;
            return true;
        case 2:
            if (c == '"')
                state = 0;
            return true;
        }
    };
}
} // namespace core

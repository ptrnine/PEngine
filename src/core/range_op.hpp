#pragma once

#include "range.hpp"

namespace core
{
constexpr inline auto oneline_comment_start(auto disabler) {
    return [disabler](const auto& b, const auto& e) mutable {
        auto c = *b;
        if (!disabler(c)) {
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

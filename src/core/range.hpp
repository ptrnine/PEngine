#pragma once

#include "types.hpp"

namespace core
{
namespace detail {
    template <typename T>
    concept AllowSpan = requires (const T& v) {
        { span{v, v - v} };
    };

    template <typename F, typename I>
    concept RangeFunc = requires (F func, I iter) {
        func(iter, iter);
    };
}

template <typename I>
struct range {
    range(I ibegin, I iend): _begin(ibegin), _end(iend) {}
    I               _begin, _end;
    [[nodiscard]] I begin() const {
        return _begin;
    }
    [[nodiscard]] I end() const {
        return _end;
    }
};
} // namespace core

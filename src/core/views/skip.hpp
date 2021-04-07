#pragma once

#include "../range.hpp"

namespace core::views
{
template <typename F, typename I>
struct skip_when_iterator {
    skip_when_iterator(F binary_op, I ibegin, I iend): b(move(ibegin)), e(move(iend)), op(move(binary_op)) {
        while (b != e && op(*b))
            ++b;
    }

    skip_when_iterator(const skip_when_iterator& iter) = default;
    skip_when_iterator& operator=(const skip_when_iterator& iter) {
        b = iter.b;
        e = iter.e;
    }

    auto operator*() {
        return *b;
    }

    auto operator->() {
        return *b;
    }

    skip_when_iterator& operator++() {
        if (b == e)
            return *this;

        ++b;
        while (b != e && op(*b))
            ++b;
        return *this;
    }

    skip_when_iterator operator++(int) {
        auto res = *this;
        ++(*this);
        return res;
    }

    [[nodiscard]]
    friend bool operator==(const skip_when_iterator& rhs, const skip_when_iterator& lhs) {
        return rhs.b == lhs.b;
    }

    I b, e;
    F op;

    using reference = decltype(*b);
    using value_type = std::decay_t<decltype(*b)>;
};

template <typename F>
struct skip_when {
    skip_when(F binary_op): _op(move(binary_op)) {}

    template <typename I>
    auto operator()(I begin, I end) {
        return range(skip_when_iterator(_op, begin, end), skip_when_iterator(_op, end, end));
    }

    friend auto operator/(Range auto&& container, skip_when skip_when) {
        return skip_when(begin(container), end(container));
    }

    F _op;
};
} // namespace core::views

template <typename F, typename I>
struct std::iterator_traits<core::views::skip_when_iterator<F, I>> {
    using value_type        = typename core::views::skip_when_iterator<F, I>::value_type;
    using reference         = typename core::views::skip_when_iterator<F, I>::reference;
    using difference_type   = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
};

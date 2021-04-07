#pragma once

#include "../range.hpp"

namespace core::views
{
template <typename I, typename C, typename... Cs>
struct drop_after_iterator {
    void try_drop() {
        if (b == e)
            return;

        hana::for_each(_after, [&]<typename T>(T& after) {
            if constexpr (Range<T>) {
                auto after_b = core::begin(after);
                auto after_e = core::end(after);
                auto tmp_b = b;

                for (; after_b != after_e && tmp_b != e; ++after_b, ++tmp_b) {
                    if (*after_b != *tmp_b)
                        break;
                }

                if (after_b == after_e)
                    _dropped = true;
            } else {
                if (*b == after)
                    _dropped = true;
            }
        });
    }

    drop_after_iterator(I ibegin, I iend, hana::tuple<C, Cs...> after):
        b(move(ibegin)), e(move(iend)), _after(move(after)) {
        try_drop();
    }

    auto operator*() {
        return *b;
    }

    auto operator->() {
        return *b;
    }

    drop_after_iterator& operator++() {
        if (b == e || _dropped)
            return *this;

        ++b;
        try_drop();

        return *this;
    }

    drop_after_iterator operator++(int) {
        auto res = *this;
        ++(*this);
        return res;
    }

    [[nodiscard]]
    friend bool operator==(const drop_after_iterator& rhs, const drop_after_iterator& lhs) {
        return rhs._dropped || lhs._dropped || rhs.b == lhs.b;
    }

    I b, e;
    hana::tuple<C, Cs...> _after;
    bool _dropped = false;

    using reference = decltype(*b);
    using value_type = std::decay_t<decltype(*b)>;
};

template <typename C, typename... Cs>
struct drop_after {
    drop_after(C after, Cs... afters): _after(move(after), move(afters)...) {}

    template <typename I>
    auto operator()(I begin, I end) {
        return range(drop_after_iterator(move(begin), move(end), _after),
                     drop_after_iterator(move(end), move(end), _after));
    }

    friend auto operator/(Range auto&& container, drop_after drop_after) {
        return drop_after(begin(container), end(container));
    }

    hana::tuple<C, Cs...> _after;
};

template <typename I, typename F>
struct drop_when_iterator {
    drop_when_iterator(I ibegin, I iend, F binary_op):
        b(move(ibegin)), e(move(iend)), _op(move(binary_op)) {
        if (b != e)
            analyze();
    }

    auto operator*() {
        return *b;
    }

    auto operator*() const {
        return *b;
    }

    auto operator->() {
        return *b;
    }

    auto operator->() const {
        return *b;
    }

    void analyze() {
         if constexpr (detail::RangeFunc<F, I>) {
            if (_dropped || _op(b, e))
                _dropped = true;
        } else {
            if (_dropped || _op(*b))
                _dropped = true;
        }
    }

    drop_when_iterator& operator++() {
        if (!_dropped && b != e) {
            ++b;
            analyze();
        }
        return *this;
    }

    drop_when_iterator operator++(int) {
        auto res = *this;
        ++(*this);
        return res;
    }

    [[nodiscard]]
    friend bool operator==(const drop_when_iterator& rhs, const drop_when_iterator& lhs) {
        return rhs._dropped || lhs._dropped || rhs.b == lhs.b;
    }

    I b, e;
    F _op;
    bool _dropped = false;

    using reference = decltype(*b);
    using value_type = std::decay_t<decltype(*b)>;
};

template <typename F>
struct drop_when {
    drop_when(F binary_op): _op(move(binary_op)) {}

    template <typename I>
    auto operator()(I begin, I end) {
        return range(drop_when_iterator(begin, end, _op),
                     drop_when_iterator(end, end, _op));
    }

    friend auto operator/(Range auto&& container, drop_when drop_when) {
        return drop_when(begin(container), end(container));
    }

    F _op;
};
} // namespace core::views

template <typename I, typename C, typename... Cs>
struct std::iterator_traits<core::views::drop_after_iterator<I, C, Cs...>> {
    using value_type        = typename core::views::drop_after_iterator<I, C, Cs...>::value_type;
    using reference         = typename core::views::drop_after_iterator<I, C, Cs...>::reference;
    using difference_type   = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
};

template <typename I, typename F>
struct std::iterator_traits<core::views::drop_when_iterator<I, F>> {
    using value_type        = typename core::views::drop_when_iterator<I, F>::value_type;
    using reference         = typename core::views::drop_when_iterator<I, F>::reference;
    using difference_type   = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
};

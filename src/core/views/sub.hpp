#pragma once

#include "../range.hpp"

namespace core::views
{
template <typename DerivedT, typename F, typename I>
struct sub_iterator_derived {
    sub_iterator_derived(I iter, F operation): i(move(iter)), op(move(operation)) {}

    auto operator*() {
        return *i / op;
    }

    auto operator->() {
        return *i / op;
    }

    auto operator*() const {
        return *i / op;
    }

    auto operator->() const {
        return *i / op;
    }

    DerivedT& operator++() {
        ++i;
        return static_cast<DerivedT&>(*this);
    }

    DerivedT operator++(int) {
        auto res = *this;
        ++(*this);
        return static_cast<DerivedT>(res);
    }

    [[nodiscard]]
    friend bool operator==(const DerivedT& rhs, const DerivedT& lhs) {
        return rhs.i == lhs.i;
    }

    I i;
    F op;

    using value_type = std::decay_t<decltype(*i / op)>;
};

template <typename F, typename I>
struct sub_iterator_input : public sub_iterator_derived<sub_iterator_input<F, I>, F, I> {
    using sub_iterator_derived<sub_iterator_input<F, I>, F, I>::sub_iterator_derived;
};

template <typename F, typename I>
struct sub_iterator_bidirect : public sub_iterator_derived<sub_iterator_bidirect<F, I>, F, I> {
    using sub_iterator_derived<sub_iterator_bidirect<F, I>, F, I>::sub_iterator_derived;

    sub_iterator_bidirect& operator--() {
        --this->i;
        return *this;
    }

    sub_iterator_bidirect operator--(int) {
        auto res = *this;
        --(*this);
        return res;
    }
};

template <typename F>
struct sub {
    sub(F operation): _op(move(operation)) {}

    template <typename I>
    requires InputRange<range<I>>
    auto operator()(I begin, I end) {
        return range(sub_iterator_input<F, decltype(begin)>(begin, _op),
                     sub_iterator_input<F, decltype(end)>(end, _op));
    }

    template <typename I>
    requires BiderectRange<range<I>>
    auto operator()(I begin, I end) {
        return range(sub_iterator_bidirect<F, decltype(begin)>(begin, _op),
                     sub_iterator_bidirect<F, decltype(end)>(end, _op));
    }

    friend auto operator/(Range auto&& container, sub sub) {
        return sub(begin(container), end(container));
    }

    F _op;
};
} // namespace core::views

template <typename F, typename I>
struct std::iterator_traits<core::views::sub_iterator_input<F, I>> {
    using value_type        = typename core::views::sub_iterator_input<F, I>::value_type;
    using reference         = value_type;
    using difference_type   = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
};
template <typename F, typename I>
struct std::iterator_traits<core::views::sub_iterator_bidirect<F, I>> {
    using value_type        = typename core::views::sub_iterator_bidirect<F, I>::value_type;
    using reference         = value_type;
    using difference_type   = ptrdiff_t;
    using iterator_category = std::bidirectional_iterator_tag;
};

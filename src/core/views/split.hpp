#pragma once

#include "../range.hpp"

namespace core::views {

namespace details {
    template <typename I>
    concept StringViewConstruct = requires(I b) {
        string_view(b, b);
    };
}

template <typename I, size_t Ndelims>
class split_iterator {
public:
    using non_const_t = std::decay_t<decltype(*std::declval<I>())>;

    template <typename TT>
    static auto make_range(TT ibegin, TT iend) {
        return range(ibegin, iend);
    }

    using value_type = decltype(make_range(std::declval<I>(), std::declval<I>()));
    using reference = value_type;

    split_iterator(I                                  start,
                   I                                  iend,
                   const array<non_const_t, Ndelims>& idelims,
                   bool                               iallow_empty = false):
        b(start), e(iend), end(iend), delims(idelims), allow_empty(iallow_empty) {
        init();
    }

    split_iterator& operator++() {
        next();
        return *this;
    }

    split_iterator operator++(int) {
        auto result = *this;
        ++(*this);
        return result;
    }

    auto operator*() {
        return make_range(b, e);
    }

    [[nodiscard]]
    friend bool operator==(const split_iterator& rhs, const split_iterator& lhs) {
        assert(rhs.delims == lhs.delims && rhs.allow_empty == lhs.allow_empty);
        return rhs.b == lhs.b;
    }

private:
    static bool has_delim(const array<non_const_t, Ndelims>& delims, const non_const_t& c) {
        return std::any_of(
            delims.begin(), delims.end(), [&c](const non_const_t& d) { return d == c; });
    }

    void init() {
        if (!allow_empty)
            while (b != end && has_delim(delims, *b))
                ++b;
        e = b;
        while (e != end && !has_delim(delims, *e))
            ++e;
    }

    void next() {
        bool repeat = true;

        while (repeat) {
            b = e;
            if (b == end)
                return;

            e = ++b;
            while (e != end && !has_delim(delims, *e))
                ++e;

            repeat = !allow_empty && b == e;
        }
    }

private:
    I b, e, end;
    array<non_const_t, Ndelims> delims;
    bool allow_empty = false;
};


template <typename I, size_t Ndelims>
class split_viewer {
public:
    using value_type = std::remove_reference_t<decltype(*std::declval<I>())>;
    using T = std::remove_const_t<value_type>;

    split_viewer(Range auto&&             container,
                 const array<T, Ndelims>& delims,
                 bool                     allow_empty = false):
        b(core::begin(container), core::end(container), delims, allow_empty),
        e(core::end(container), core::end(container), delims, allow_empty) {}

    split_iterator<I, Ndelims> begin() {
        return b;
    }

    split_iterator<I, Ndelims> end() {
        return e;
    }

private:
    split_iterator<I, Ndelims> b, e;
};

enum class split_mode : u8 {
    skip_empty,
    allow_empty
};

template <typename ContainerT, typename T, size_t Ndelims>
split_viewer(ContainerT&&,
             const array<T, Ndelims>&,
             bool) -> split_viewer<decltype(begin(std::declval<ContainerT>())), Ndelims>;

template <typename T, size_t Ndelims>
struct split {
    template <typename C, typename... Cs>
    split(C delim, Cs... delims):
        _delims{move(delim), move(delims)...}, _mode(split_mode::skip_empty) {}

    template <typename C, typename... Cs>
    split(split_mode mode, C delim, Cs... delims):
        _delims{move(delim), move(delims)...}, _mode(mode) {}

    template <Range Rng>
    friend auto operator/(Rng&& container, const split& sv) {
        return split_viewer(forward<Rng>(container), sv._delims, sv._mode == split_mode::allow_empty);
    }

    array<T, Ndelims> _delims;
    split_mode _mode;
};

template <typename C, typename... Cs>
split(C, Cs...) -> split<C, sizeof...(Cs) + 1>;

template <typename C, typename... Cs>
split(split_mode, C, Cs...) -> split<C, sizeof...(Cs) + 1>;

} // namespace core::views

template <typename T, size_t Ndelims>
struct std::iterator_traits<core::views::split_iterator<T, Ndelims>> {
    using value_type = typename core::views::split_iterator<T, Ndelims>::value_type;
    using reference = value_type;
    using difference_type = ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;
};

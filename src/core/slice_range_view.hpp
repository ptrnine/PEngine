#pragma once

#include "types.hpp"

namespace core
{
namespace helper {
    using std::get;
    using std::begin;
    using std::end;
    using std::next;
}

template <typename T>
concept SliceRangeTupleLike = requires(T&& v) {
    {helper::get<0>(v)} -> std::convertible_to<size_t>;
    {helper::get<1>(v)} -> std::convertible_to<size_t>;
};

template <typename T>
concept SliceRangeIter = requires(T&& v) {
    {*helper::begin(v)} -> std::convertible_to<size_t>;
    {*helper::next(helper::begin(v))} -> std::convertible_to<size_t>;
} && !SliceRangeTupleLike<T>;


/**
 * @brief Any container with 2 elements at least
 *
 * @tparam T
 */
template <typename T>
concept SliceRange = SliceRangeTupleLike<T> || SliceRangeIter<T>;

template <SliceRangeIter T>
constexpr auto slice_range_start(const T& slice_range) {
    return *helper::begin(slice_range);
}

template <SliceRangeTupleLike T>
constexpr auto slice_range_start(const T& slice_range) {
    return helper::get<0>(slice_range);
}

template <SliceRangeIter T>
constexpr auto slice_range_size(const T& slice_range) {
    return *helper::next(helper::begin(slice_range));
}

template <SliceRangeTupleLike T>
constexpr auto slice_range_size(const T& slice_range) {
    return helper::get<1>(slice_range);
}

template <class T>
using iterator_t = decltype(helper::begin(std::declval<T&>()));

template <typename T>
concept SliceRangeIterator = std::bidirectional_iterator<T> && SliceRange<std::iter_value_t<T>>;

template <typename T>
concept SliceRangeRange = SliceRangeIterator<iterator_t<T>>;

template <typename T>
struct slice_range_helper {
    slice_range_helper() = default;
    slice_range_helper(T b, T e): _b(move(b)), _e(move(e)) {}
    constexpr auto begin() const { return _b; }
    constexpr auto end() const { return _e; }
    T _b, _e;
};

template <typename T, SliceRangeIterator I>
class slice_range_view_iterator {
public:
    using value_type = slice_range_helper<iterator_t<T>>;
    using difference_type = std::iter_difference_t<I>;

    slice_range_view_iterator() = default;
    ~slice_range_view_iterator() = default;
    slice_range_view_iterator(const slice_range_view_iterator&) = default;
    slice_range_view_iterator& operator=(const slice_range_view_iterator&) = default;
    slice_range_view_iterator(slice_range_view_iterator&&) noexcept = default;
    slice_range_view_iterator& operator=(slice_range_view_iterator&&) noexcept = default;

    slice_range_view_iterator(T* data, I start): _data(data), _i(start) {}

    slice_range_view_iterator& operator++() {
        ++_i;
        return *this;
    }

    slice_range_view_iterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    slice_range_view_iterator& operator--() {
        --_i;
        return *this;
    }

    slice_range_view_iterator operator--(int) {
        auto tmp = *this;
        --(*this);
        return tmp;
    }

    value_type operator*() const {
        auto start = slice_range_start(*_i);
        auto size  = slice_range_size(*_i);
        return slice_range_helper{helper::begin(*_data) + start, helper::begin(*_data) + start + size};
    }

    bool operator==(const slice_range_view_iterator& rhs) const {
        return _data == rhs._data && _i == rhs._i;
    }

    bool operator!=(const slice_range_view_iterator& rhs) const {
        return !(_i == rhs._i);
    }

    slice_range_view_iterator& operator+=(difference_type value) {
        _i += value;
        return *this;
    }

    slice_range_view_iterator operator+(difference_type value) const {
        auto tmp = *this;
        tmp += value;
        return tmp;
    }

    slice_range_view_iterator& operator-=(difference_type value) {
        _i -= value;
        return *this;
    }

    slice_range_view_iterator operator-(difference_type value) const {
        auto tmp = *this;
        tmp -= value;
        return tmp;
    }

    bool operator>=(const slice_range_view_iterator& rhs) const {
        return _data == nullptr || (_i > rhs._i && rhs._data != nullptr);
    }

    bool operator<(const slice_range_view_iterator& rhs) const {
        return !(*this >= rhs);
    }

    bool operator<=(const slice_range_view_iterator& rhs) const {
        return rhs._data == nullptr || (rhs._i > _i && _data != nullptr);
    }

    bool operator>(const slice_range_view_iterator& rhs) const {
        return !(*this <= rhs);
    }

    difference_type operator-(const slice_range_view_iterator& rhs) const {
        return _i - rhs._i;
    }

private:
    T* _data = nullptr;
    I  _i;
    value_type _slice;
};

/**
 * @brief Creates the range of slices
 *
 * For example:
 *
 * auto v = vector{0, 1, 2, 3, 4, 5, 6, 7};
 * auto slice_spec = vector{pair{0, 3}, pair{3, 5}, pair{1, 2}};
 * for (auto slice : slice_range_view(v, slice_spec)) {
 *     for (auto& element : slice)
 *         cout << element << " ";
 *     cout << std::endl;
 * }
 *
 * This prints:
 * 0 1 2
 * 3 4 5 6 7
 * 1 2
 *
 * @tparam T - type of container
 * @tparam R - type of slices parameters (vector/array of vector/array/pair with at least 2 elements)
 * @param data - the input container
 * @param slices - container with slices
 *
 * @return the range of slices
 */
template <typename T, SliceRangeRange R>
auto slice_range_view(T& data, R& slices) {
    auto beg = slice_range_view_iterator(&data, helper::begin(slices));
    auto end = slice_range_view_iterator(&data, helper::end(slices));
    return slice_range_helper{beg, end};
}

} // namespace core

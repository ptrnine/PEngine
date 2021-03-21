#pragma once
#include "platform_dependent.hpp"
#include "print.hpp"
#include "ston.hpp"
#include "vec.hpp"
#include <fstream>
#include <ranges>
#include <utility>

#define GENERIC_C_STRING(CHAR_T, STR)                                                                                  \
    []() constexpr {                                                                                                   \
        static_assert(std::is_same_v<CHAR_T, char> || std::is_same_v<CHAR_T, char16_t> ||                              \
                          std::is_same_v<CHAR_T, char32_t>,                                                            \
                      "Invalid character type");                                                                       \
        if constexpr (std::is_same_v<CHAR_T, char>)                                                                    \
            return STR;                                                                                                \
        else if constexpr (std::is_same_v<CHAR_T, char16_t>)                                                           \
            return u"#STR";                                                                                            \
        else if constexpr (std::is_same_v<CHAR_T, char32_t>)                                                           \
            return U"#STR";                                                                                            \
    }                                                                                                                  \
    ()

namespace core
{

template <typename size_type>
class index_view_iterator : public std::iterator<std::bidirectional_iterator_tag, size_type> {
public:
    index_view_iterator(size_type init, size_type increment = 1) noexcept: idx(init), inc(increment) {}
    index_view_iterator() = default;

    inline index_view_iterator& operator++() noexcept {
        idx+=inc;
        return *this;
    }

    inline index_view_iterator& operator--() noexcept {
        idx-=inc;
        return *this;
    }

    inline index_view_iterator operator++(int) noexcept {
        auto tmp = *this;
        idx+=inc;
        return tmp;
    }

    inline index_view_iterator operator--(int) noexcept {
        auto tmp = *this;
        idx-=inc;
        return tmp;
    }

    inline bool operator==(const index_view_iterator& i) const noexcept {
        return idx == i.idx;
    }

    inline bool operator!=(const index_view_iterator& i) const noexcept {
        return !(*this == i);
    }

    inline const size_type& operator*() const noexcept {
        return idx;
    }

    inline size_type& operator*() noexcept {
        return idx;
    }

private:
    size_type idx = 0;
    size_type inc = 1;
};

template <typename T, size_t S>
class dimensional_index_iterator : public std::iterator<std::forward_iterator_tag, T> {
public:
    dimensional_index_iterator(array<T, S> end) noexcept: ends(end) {}
    dimensional_index_iterator() noexcept: wrapped(true) {}

    inline dimensional_index_iterator& operator++() noexcept {
        increment();
        return *this;
    }

    inline dimensional_index_iterator operator++(int) noexcept {
        dimensional_index_iterator res = *this;
        increment();
        return res;
    }

    inline bool operator==(const dimensional_index_iterator& i) const noexcept {
        return idxs == i.idxs && wrapped == i.wrapped;
    }

    inline bool operator!=(const dimensional_index_iterator& i) const noexcept {
        return !(*this == i);
    }

    inline vec<T, S> operator*() const noexcept {
        return {idxs};
    }

private:
    void increment(size_t idx = 0) {
        if (idx != S) {
            ++idxs[idx];
            if (idxs[idx] == ends[idx]) {
                idxs[idx] = 0;
                increment(idx + 1);
            }
        } else
            wrapped = true;
    }

private:
    array<T, S> idxs = {0};
    array<T, S> ends;
    bool wrapped = false;
};


template <typename... Ts>
struct multi_view_tuple : tuple<Ts...> {
    using tuple<Ts...>::tuple;

    template <typename... Tts, std::enable_if_t<(std::is_constructible_v<Ts, Tts&&> && ...), int> = 0>
    multi_view_tuple(tuple<Tts...>&& t): tuple<Ts...>(std::move(t)) {}

    template <size_t N>
    std::tuple_element_t<N, multi_view_tuple<Ts...>>& get() & {
        return std::get<N>(*this);
    }

    template <size_t N>
    const std::tuple_element_t<N, multi_view_tuple<Ts...>>& get() const& {
        return std::get<N>(*this);
    }

    template <size_t N>
    std::tuple_element_t<N, multi_view_tuple<Ts...>> get() && {
        return move(std::get<N>(*this));
    }

    //template <size_t N>
    //const std::tuple_element_t<N, multi_view_tuple<Ts...>>&& get() const&& {
    //    return move(std::get<N>(*this));
    //}
};


template <typename... Ts, size_t... Idxs>
inline void multi_view_increment(tuple<Ts...>& v, std::index_sequence<Idxs...>&&) {
    (++std::get<Idxs>(v), ...);
}

template <typename... Ts, size_t... Idxs>
inline void multi_view_decrement(tuple<Ts...>& v, std::index_sequence<Idxs...>&&) {
    (--std::get<Idxs>(v), ...);
}

template <typename... Ts, size_t... Idxs>
inline bool multi_view_any_equal(const tuple<Ts...>& lhs, const tuple<Ts...>& rhs, std::index_sequence<Idxs...>) {
    return ((std::get<Idxs>(lhs) == std::get<Idxs>(rhs)) || ...);
}

template <typename... Ts, size_t... Idxs>
inline auto multi_view_map_values(tuple<Ts...>& v, std::index_sequence<Idxs...>&&) {
    return multi_view_tuple<decltype(*std::declval<Ts>())...>(*std::get<Idxs>(v)...);
}


template <typename... Is>
class multi_view_iterator {
public:
    using iterator_category = std::bidirectional_iterator_tag;

    multi_view_iterator(Is... iiterators): iterators(iiterators...) {}

    inline multi_view_iterator& operator++() {
        multi_view_increment(iterators, std::make_index_sequence<sizeof...(Is)>());
        return *this;
    }

    inline multi_view_iterator& operator--() {
        multi_view_decrement(iterators, std::make_index_sequence<sizeof...(Is)>());
        return *this;
    }

    inline multi_view_iterator operator++(int) {
        multi_view_iterator tmp = *this;
        multi_view_increment(iterators, std::make_index_sequence<sizeof...(Is)>());
        return tmp;
    }

    inline multi_view_iterator operator--(int) {
        multi_view_iterator tmp = *this;
        multi_view_decrement(iterators, std::make_index_sequence<sizeof...(Is)>());
        return tmp;
    }

    inline bool operator==(const multi_view_iterator& i) const noexcept {
        return multi_view_any_equal(iterators, i.iterators, std::make_index_sequence<sizeof...(Is)>());
    }

    inline bool operator!=(const multi_view_iterator& i) const noexcept {
        return !(*this == i);
    }

    decltype(auto) operator*() {
        value.emplace(multi_view_map_values(iterators, std::make_index_sequence<sizeof...(Is)>()));
        return *value;
    }

private:
    tuple<Is...>                                                 iterators;
    optional<multi_view_tuple<decltype(*std::declval<Is>())...>> value;
};


template <typename IterT>
class iterator_view_proxy {
public:
    iterator_view_proxy(IterT begin, IterT end) noexcept: _begin(begin), _end(end) {}

    [[nodiscard]] IterT begin() const noexcept {
        return _begin;
    }

    [[nodiscard]] IterT end() const noexcept {
        return _end;
    }

private:
    IterT _begin;
    IterT _end;
};


/**
 * @brief Iterate with indices
 *
 * Usage:
 * for (auto i : index_view(c.begin(), c.end())) {
 * }
 *
 * Equivalent:
 * for (decltype(c)::iterator::difference_type i = 0; i < (c.end() - c.begin()); ++i) {
 * }
 *
 * @tparam IterT - the type of the iterator
 * @param begin  - the start of the range
 * @param end    - the end of the range
 *
 * @return iterator_view_proxy
 */
template <typename IterT>
auto index_view(IterT begin, IterT end) {
    return iterator_view_proxy(index_view_iterator<size_t>(), index_view_iterator(static_cast<size_t>(end - begin)));
}

/**
 * @brief Iterate over two containers
 *
 * @tparam IterT1 - the type of the first iterator
 * @tparam IterT2 - the type of the second iterator
 * @param begin1  - the start of the first range
 * @param end1    - the end of the first range
 * @param begin2  - the start of the second range
 * @param end2    - the end of the second range
 *
 * @return iterator_view_proxy
 */
template <typename IterT1, typename IterT2>
auto zip_view(IterT1 begin1, IterT1 end1, IterT2 begin2, IterT2 end2) {
    return iterator_view_proxy(multi_view_iterator(begin1, begin2), multi_view_iterator(end1, end2));
}

/**
 * @brief Iterate over container with skipping from the start
 *
 * @tparam T - the type of container
 * @param container - the container
 * @param start - the count of skipped values
 *
 * @return iterator_view_proxy
 */
template <Iterable T>
auto skip_view(T& container, size_t start) {
    using difference_type = decltype(container.begin() - container.begin());
    return iterator_view_proxy(container.begin() + static_cast<difference_type>(start), container.end());
}

/**
 * @brief Iterate with values and and indices
 *
 * Usage:
 * for (auto [val, i] : value_index_view(c.begin(), c.end())) {
 * }
 *
 * Equivalent of:
 * for (decltype(c)::iterator::difference_type i = 0; i < c.size(); ++i) {
 *     auto val = c[i];
 * }
 *
 * @tparam I - the type of iterator
 * @param begin - the start of range
 * @param end - the end of range
 *
 * @return iterator_view_proxy
 */
template <typename I>
auto value_index_view(I begin, I end) {
    return zip_view(begin, end, index_view_iterator<size_t>(0), index_view_iterator(static_cast<size_t>(end - begin)));
}

/**
 * @brief Iterate with indices
 *
 * Usage:
 * for (auto i : index_view(container)) {
 * }
 *
 * Equivalent of:
 * for (decltype(container)::iterator::difference_type i = 0; i < container.size(); ++i) {
 * }
 *
 * @tparam T - the type of container
 * @param container - the container
 *
 * @return iterator_view_proxy
 */
template <Iterable T>
auto index_view(T& container) {
    return index_view(container.begin(), container.end());
}

/**
 * @brief Generate index sequence
 *
 * Usage:
 * for (size_t i : index_view(100)) {
 * }
 *
 * Equivalent of:
 * for (size_t i = 0; i < 100; ++i) {
 * }
 *
 * @tparam - type of index
 * @param stop - an number specifying at which position to stop (not included)
 * @return - sequence of indicies
 *
 */
template <typename T = ptrdiff_t>
auto index_seq(T stop) {
    return iterator_view_proxy(index_view_iterator<T>(), index_view_iterator<T>(stop));
}

/**
 * @brief Generate index sequence
 *
 * Usage:
 * for (size_t i : index_view(100)) {
 * }
 *
 * Equivalent of:
 * for (size_t i = 0; i < 100; ++i) {
 * }
 *
 * @param stop - an number specifying at which position to stop (not included)
 * @return - sequence of indicies
 *
 */
inline auto uindex_seq(size_t stop) {
    return index_seq<size_t>(stop);
}

/**
 * @brief Generate index sequence
 *
 * Usage:
 * for (size_t i : index_view(0, 100, 2)) {
 * }
 *
 * Equivalent of:
 * for (size_t i = 0; i < 100; i+=2) {
 * }
 *
 * @tparam T    - type of index
 * @param start - an number specifying at which position to start
 * @param stop  - an number specifying at which position to stop (not included)
 * @param step  - an number specifying the incrementation, default is 1
 * @return - sequence of indicies
 *
 */
template <typename T = ptrdiff_t>
auto index_seq(T start, T stop, T step = 1) {
    T real_stop = stop - start;
    if (real_stop % step != 0)
        real_stop = stop - real_stop % step + step;
    else
        real_stop = stop;
    return iterator_view_proxy(index_view_iterator<T>(start, step), index_view_iterator<T>(real_stop));
}

/**
 * @brief Generate index sequence
 *
 * Usage:
 * for (size_t i : index_view(0, 100, 2)) {
 * }
 *
 * Equivalent of:
 * for (size_t i = 0; i < 100; i+=2) {
 * }
 *
 * @tparam T    - type of index
 * @param start - an number specifying at which position to start
 * @param stop  - an number specifying at which position to stop (not included)
 * @param step  - an number specifying the incrementation, default is 1
 * @return - sequence of indicies
 *
 */
inline auto uindex_seq(size_t start, size_t stop, size_t step = 1) {
    return index_seq<size_t>(start, stop, step);
}

/**
 * @brief Generate n-dimensional index sequence
 *
 * Usage:
 * for (size_t [x, y] : dimensional_seq(10, 20)) {
 * }
 *
 * Equivalent of:
 * for (size_t y = 0; y < 20; ++y) {
 *     for (size_t x = 0; x < 10; ++x) {
 *     }
 * }
 *
 * @tparam T    - type of index
 * @param x_max - an number specifying at which position to stop for first dimension (not included)
 * @param n_max - another numbers specifying at which position to stop (not included)
 * @return      - n-dimensional index sequence
 *
 */
template <typename T = size_t, typename... Ts>
auto dimensional_seq(size_t x_max, Ts... n_max) {
    return iterator_view_proxy(
            dimensional_index_iterator(array<T, sizeof...(Ts) + 1>{x_max, static_cast<T>(n_max)...}),
            dimensional_index_iterator<T, sizeof...(Ts) + 1>());
}

/**
 * @brief Generate n-dimensional index sequence
 *
 * Usage:
 * for (int [x, y] : dimensional_seq(vec{10, 20})) {
 * }
 *
 * Equivalent of:
 * auto v = vec{10, 20};
 * for (int y = 0; y < vec.y(); ++y) {
 *     for (size_t x = 0; x < vec.x(); ++x) {
 *     }
 * }
 *
 * @tparam T  - type of index
 * @param max - n-dimensional vector specifying at which positions to stop (not included)
 * @return    - n-dimensional index sequence
 *
 */
template <typename T = size_t, size_t S>
auto dimensional_seq(vec<T, S> maxs) {
    return iterator_view_proxy(
            dimensional_index_iterator(maxs.v),
            dimensional_index_iterator<T, S>());
}

/**
 * @brief Iterate over two or more containers
 *
 * Usage:
 * for (auto [val1, val2, val3] : zip_view(container1, container2, container3)) {
 * }
 *
 * Equivalent of:
 * for (size_t i = 0; i < min(container1.size(), container2.size(), container3.size()); ++i) {
 *     auto val1 = container1[i];
 *     auto val2 = container2[i];
 *     auto val3 = container3[i];
 * }
 *
 * @tparam Ts - types of containers
 * @param containers - containers
 *
 * @return iterator_view_proxy
 */
template <Iterable... Ts>
auto zip_view(Ts&... containers) {
    return iterator_view_proxy(multi_view_iterator(containers.begin()...), multi_view_iterator(containers.end()...));
}

/**
 * @brief Iterate with values and and indices
 *
 * Usage:
 * for (auto [val, i] : value_index_view(c)) {
 * }
 *
 * Equivalent of:
 * for (decltype(c)::iterator::difference_type i = 0; i < c.size(); ++i) {
 *     auto val = c[i];
 * }
 *
 * @tparam T - the type of the container
 * @param container - the container
 *
 * @return iterator_view_proxy
 */
template <Iterable T>
auto value_index_view(T& container) {
    return zip_view(
            container.begin(),
            container.end(),
            index_view_iterator<size_t>(0),
            index_view_iterator(static_cast<size_t>(container.end() - container.begin())));
}

template <typename ContainerT, typename T>
requires Iterable<ContainerT> inline auto _fold_operator(const ContainerT& t, const T& delimiter) {
    using ItemT = std::remove_const_t<std::decay_t<decltype(t[0])>>;

    if (std::empty(t))
        return ItemT();
    else {
        ItemT result;
        auto  inserter = std::back_inserter(result);
        auto  begin    = std::begin(t);
        auto  end      = std::end(t);

        copy(std::begin(*begin), std::end(*begin), inserter);
        ++begin;

        while (begin != end) {
            copy(std::begin(delimiter), std::end(delimiter), inserter);
            copy(std::begin(*begin), std::end(*begin), inserter);
            ++begin;
        }

        return result;
    }
}

template <typename F>
inline void generate(Iterable auto& container, F&& generator) {
    std::generate(std::begin(container), std::end(container), std::forward<F>(generator));
}

inline void sort(Iterable auto& container) {
    std::sort(std::begin(container), std::end(container));
}

inline bool is_unique_sort(Iterable auto& container) {
    std::sort(std::begin(container), std::end(container));
    auto pos = std::adjacent_find(std::begin(container), std::end(container));
    return pos == std::end(container);
}

/**
 * @brief Adapter for std::count_if
 */
template <typename F>
struct count_if {
    using adapter = void;

    count_if(F callback): functor(std::move(callback)) {}

    template <Iterable T>
    auto operator()(const T& c) {
        return std::count_if(std::begin(c), std::end(c), functor);
    }

    F functor;
};

/**
 * @brief Adapter for std::any_of
 */
template <typename F>
struct any_of {
    using adapter = void;

    any_of(F callback): functor(std::move(callback)) {}

    template <Iterable T>
    bool operator()(const T& c) {
        return std::any_of(std::begin(c), std::end(c), functor);
    }

    F functor;
};

/**
 * @brief Adapter for std::all_of
 */
template <typename F>
struct all_of {
    using adapter = void;

    all_of(F callback): functor(std::move(callback)) {}

    template <Iterable T>
    bool operator()(const T& c) {
        return std::all_of(std::begin(c), std::end(c), functor);
    }

    F functor;
};

/**
 * @brief Adapter for std::find_if
 */
template <typename F>
struct find_if {
    using adapter = void;

    find_if(F callback): functor(std::move(callback)) {}

    template <Iterable TT>
    auto operator()(const TT& c) {
        return std::find_if(std::begin(c), std::end(c), functor);
    }

    F functor;
};

template <typename C, typename F>
struct transform_t {
    using adapter = void;

    transform_t(F callback): functor(std::move(callback)) {}

    template <RandomAccessIterable T> requires Resizable<C> && IndexAccessible<C>
    C operator()(const T& c) {
        C r;
        r.resize(static_cast<size_t>(std::end(c) - std::begin(c)));
        std::transform(std::begin(c), std::end(c), std::begin(r), functor);
        return r;
    }

    template <RandomAccessIterable T> requires IndexAccessible<C> && std::is_trivial_v<C>
    C operator()(const T& c) {
        C r;
        std::transform(std::begin(c), std::end(c), std::begin(r), functor);
        return r;
    }

    F functor;
};

/**
 * @brief Adapter for std::transform
 *
 * @tparam - the type of new container
 */
template <typename C, typename F>
transform_t<C, F> transform(F functor) {
    return transform_t<C, F>(functor);
}

/**
 * @brief Adapter for std::is_sorted
 */
template <typename F = std::less<>>
struct is_sorted {
    using adapter = void;

    is_sorted(F callback = F()): functor(std::move(callback)) {}

    template <Iterable T>
    bool operator()(const T& c) {
        return std::is_sorted(std::begin(c), std::end(c), functor);
    }

    F functor;
};

/**
 * @brief Adapter for std::adjacent_difference
 */
template <typename F = std::minus<>>
struct adjacent_difference {
    using adapter = void;

    adjacent_difference(F callback = std::minus<>()): functor(callback) {}

    template <Iterable T>
    T operator()(const T& c) {
        T r = c;
        std::adjacent_difference(std::begin(r), std::end(r), std::begin(r), functor);
        return r;
    }

private:
    F functor;
};

/**
 * @brief Adapter for std::reduce
 */
template <typename T, typename F>
struct reduce {
    using adapter = void;

    template <Iterable TT>
    auto operator()(const TT& c) {
        return std::reduce(std::begin(c), std::end(c), result, functor);
    }

    reduce(T initial, F callback): result(initial), functor(callback) {}

private:
    T result;
    F functor;
};

/**
 * @brief Adapter for std::accumulate
 */
template <typename T, typename F>
struct accumulate {
    using adapter = void;

    template <Iterable TT>
    auto operator()(const TT& c) {
        return std::accumulate(std::begin(c), std::end(c), result, functor);
    }

    accumulate(T initial, F callback): result(initial), functor(callback) {}

private:
    T result;
    F functor;
};

/**
 * @brief Adapter for folding
 */
template <typename T>
struct fold {
    using adapter = void;

    template <typename ContainerT>
    auto operator()(ContainerT&& c) {
        if constexpr (Iterable<T>)
            return _fold_operator(std::forward<ContainerT>(c), delimiter);
        else {
            std::decay_t<decltype(c[0])> d = {delimiter};
            return _fold_operator(std::forward<ContainerT>(c), d);
        }
    }

    fold(T delim): delimiter(std::move(delim)) {}

    T delimiter;
};

/**
 * @brief Adapter for folding
 */
template <typename T>
struct fold<const T*> {
    using adapter = void;

    template <typename ContainerT>
    auto operator()(const ContainerT& c) {
        return _fold_operator(c, std::basic_string_view(delimiter));
    }

    explicit fold(const T* delim): delimiter(delim) {}

    const T* delimiter;
};

/**
 * @brief Adapter for casting a string to number
 */
template <typename NumberT = int>
struct to_number {
    using adapter = void;

    template <typename T>
    NumberT operator()(const T& str) {
        return ston<NumberT>(str);
    }
};

template <typename T1, typename T2>
struct replace {
    using adapter = void;

    replace(T1 iwhat, T2 iby): what(std::move(iwhat)), by(std::move(iby)) {}

    template <typename U>
    U operator()(const U& v) {
        using std::begin, std::end;
        auto v_begin = begin(v), v_end = end(v);
        auto w_begin = begin(what), w_end = end(what);
        auto mid = std::search(v_begin, v_end, w_begin, w_end);

        if (mid == v_end)
            return v;

        auto b_begin = begin(by), b_end = end(by);

        U result;
        result.resize(
            static_cast<size_t>((v_end - v_begin) - (w_end - w_begin) + (b_end - b_begin)));
        auto r_begin = begin(result);

        auto pos = std::copy(v_begin, mid, r_begin);
        pos = std::copy(b_begin, b_end, pos);
        std::copy(mid + (w_end - w_begin), v_end, pos);

        return result;
    }

    T1 what;
    T2 by;
};

namespace details {
    template <typename T>
    concept AnyChar = std::same_as<T, char> || std::same_as<T, char16_t> ||
                      std::same_as<T, char32_t> || std::same_as<T, char8_t>;
}

template <details::AnyChar T1, size_t N1, details::AnyChar T2, size_t N2>
replace(const T1 (&)[N1], const T2 (&)[N2]) // NOLINT
    -> replace<std::basic_string_view<T1>, std::basic_string_view<T2>>;

/**
 * @brief Adapter for removing trailing whitespaces
 */
struct remove_trailing_whitespaces {
    using adapter = void;

    template <typename T>
    T operator()(const T& str) {
        if (str.empty())
            return {};

        typename T::size_type start = 0;
        typename T::size_type end   = str.size() - 1;

        for (; start < str.size() && (str[start] == ' ' || str[start] == '\t'); ++start);
        for (; start < end && (str[end] == ' ' || str[end] == '\t'); --end);

        return str.substr(start, end - start + 1);
    }

    string_view operator()(const char* str) {
        return operator()(string_view(str));
    }
};

/**
 * @brief Adapter for splitting string into vector<string>
 */
template <typename CharT, typename StrT = std::basic_string<CharT>>
struct split {
    using adapter = void;

    template <typename T>
    requires StringLike<T> vector<StrT> operator()(const T& str) {
        vector<StrT>                  vec;
        std::basic_string_view<CharT> data  = str;
        size_t                        start = 0;

        for (size_t i = 0; i < data.size(); ++i) {
            bool cmp = false;
            for (auto c : delims) {
                if (data[i] == c) {
                    cmp = true;
                    break;
                }
            }
            if (cmp) {
                if (create_empty_strings || start != i)
                    vec.emplace_back(data.substr(start, i - start));
                start = i + 1;
            }
        }
        if (start != data.size())
            vec.emplace_back(data.substr(start, data.size()));

        return vec;
    }

    split(const vector<CharT>& delimiters, bool should_create_empty_strings = false):
        delims(delimiters), create_empty_strings(should_create_empty_strings) {}

    split(std::initializer_list<CharT> delimiters, bool should_create_empty_strings = false):
        delims(delimiters), create_empty_strings(should_create_empty_strings) {}

    split(CharT delimiter, bool should_create_empty_strings = false):
        delims(1, delimiter), create_empty_strings(should_create_empty_strings) {}

    vector<CharT> delims;
    bool          create_empty_strings = false;
};

/**
 * Adapter for splitting string into vector<string_view>
 */
template <typename CharT>
struct split_view : public split<CharT, std::basic_string_view<CharT>> {
    explicit split_view(const vector<CharT>& delimiters, bool should_create_empty_strings = false):
        split<CharT, std::basic_string_view<CharT>>(delimiters, should_create_empty_strings) {}

    split_view(std::initializer_list<CharT> delimiters, bool should_create_empty_strings = false):
        split<CharT, std::basic_string_view<CharT>>(delimiters, should_create_empty_strings) {}

    explicit split_view(CharT delimiter, bool should_create_empty_strings = false):
        split<CharT, std::basic_string_view<CharT>>(delimiter, should_create_empty_strings) {}
};

template <typename CharT>
inline std::basic_string<CharT> path_eval(std::basic_string_view<CharT> src) {
    if (src.empty())
        return std::basic_string<CharT>();

    auto has_slash_prefix  = src.front() == static_cast<CharT>('/');
    auto has_slash_postfix = src.back() == static_cast<CharT>('/');

    auto src_begin = has_slash_prefix ? src.begin() + 1 : src.begin();
    auto src_end   = has_slash_postfix ? src.end() - 1 : src.end();

    if (src_end < src_begin)
        return std::basic_string<CharT>(GENERIC_C_STRING(CharT, "/"));

    auto splits = std::basic_string_view(
                      src_begin, static_cast<typename std::basic_string_view<CharT>::size_type>(src_end - src_begin)) /
                  split_view(static_cast<CharT>('/'), true);
    auto last_inner_double_slash =
        std::find_if(splits.rbegin(), splits.rend(), [](const auto& s) { return s.empty(); });

    auto begin = (last_inner_double_slash != splits.rend()) ? has_slash_prefix = true,
        splits.begin() + std::distance(last_inner_double_slash, splits.rend()) : // -1 was missed because we get token after "/"
        splits.begin();

    std::vector<std::basic_string_view<CharT>> result;

    for (; begin != splits.end(); ++begin) {
        if (*begin == GENERIC_C_STRING(CharT, "..")) {
            if (!result.empty() && result.back() != GENERIC_C_STRING(CharT, "..")) {
                result.pop_back();

                if (result.empty())
                    has_slash_prefix = true;
            }
            else {
                result.emplace_back(*begin);
            }
        }
        else if (*begin == GENERIC_C_STRING(CharT, "~")) {
            result.clear();
            result.emplace_back(*begin);
        }
        else if (*begin != GENERIC_C_STRING(CharT, ".")) {
            result.emplace_back(*begin);
        }
    }

    std::basic_string<CharT> fold;

    auto result_begin = result.begin();
    auto result_end   = result.end();

    if (*result_begin == GENERIC_C_STRING(CharT, "~")) {
        // Todo: Cast to CharT encoding
        fold.append(platform_dependent::get_home_dir());

        if (fold.back() != static_cast<CharT>('/'))
            fold.push_back(static_cast<CharT>('/'));

        ++result_begin;
    }
    else if (has_slash_prefix) {
        fold.push_back(static_cast<CharT>('/'));
    }

    for (; result_begin != result_end; ++result_begin) {
        fold.append(*result_begin);
        fold.push_back(static_cast<CharT>('/'));
    }

    if (!has_slash_postfix)
        fold.pop_back();

    return fold;
}

/**
 * @brief Evaluates path
 *
 * Evaluates path with '..', '.', '~/'
 *
 * Example:
 * path_eval("~/dir")              ->  "/home/username/dir"
 * path_eval("/dir1/dir2/../dir3") ->  "/dir1/dir3"
 *
 * Be careful with double slash:
 * path_eval("/foo//bar/baz/")     ->  "/bar/baz/"
 *
 *
 * @param path - the string with path
 *
 * @return evaluated path
 */
[[nodiscard]]
inline string path_eval(string_view path) {
    return path_eval<char>(path);
}

using platform_dependent::basename;

/**
 * @brief Case insensitive match of two ASCII strings
 *
 * @param str1 - the first string
 * @param str2 - the second string
 *
 * @return true if strings are equal, false otherwise
 */
[[nodiscard]]
inline bool case_insensitive_match(string_view str1, string_view str2) {
    auto a = std::string(str1);
    auto b = std::string(str2);

    std::transform(a.begin(), a.end(), a.begin(), ::tolower);
    std::transform(b.begin(), b.end(), b.begin(), ::tolower);

    return a == b;
}

template <typename CharT>
inline std::basic_string<CharT> operator/(std::basic_string_view<CharT> path1, std::basic_string_view<CharT> path2) {
    std::basic_string result(path1);

    if ((result.empty() || result.back() != '/') && (path2.empty() || path2.front() != '/')) {
        result.push_back('/');
        result.append(path2);
    }
    else if ((!result.empty() && result.back() == '/') && (!path2.empty() && path2.front() == '/')) {
        result.pop_back();
        result.append(path2);
    }
    else {
        result.append(path2);
    }

    return result;
}

/**
 * @brief Path append operator
 *
 * @param path1 - the path
 * @param path2 - the path to be appended
 *
 * @return a new path
 */
inline core::string operator/(core::string_view path1, core::string_view path2) {
    return operator/<char>(path1, path2);
}

template <typename T, size_t S, typename F, size_t... Idxs>
constexpr inline auto array_map(const array<T, S>& arr, F callback, std::index_sequence<Idxs...>&&) {
    return array{callback(std::get<Idxs>(arr))...};
}

/**
 * @brief Compile-time map for array
 *
 * @tparam T - the type of the value
 * @tparam S - the size
 * @tparam F - callback type
 * @param arr - the array
 * @param callback - the callback
 *
 * @return new array
 */
template <typename T, size_t S, typename F>
constexpr inline auto array_map(const array<T, S>& arr, F callback) {
    return array_map(arr, callback, std::make_index_sequence<S>());
}

template <typename... Ts> requires requires (const Ts&... s) { ((s.size()), ...); (string_view(s), ...); }
inline string build_string(const Ts&... strings) {
    string str;
    str.reserve((strings.size() + ...));
    ((str.append(strings)), ...);
    return str;
}
} // namespace core

namespace std
{
template <typename... Ts>
struct tuple_size<core::multi_view_tuple<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

template <size_t N, typename... Ts>
struct tuple_element<N, core::multi_view_tuple<Ts...>> : tuple_element<N, tuple<std::remove_reference_t<Ts>...>> {};
} // namespace std


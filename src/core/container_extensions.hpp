#pragma once
#include "platform_dependent.hpp"
#include "print.hpp"
#include "ston.hpp"
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
    index_view_iterator(size_type init) noexcept: idx(init) {}
    index_view_iterator() = default;

    inline index_view_iterator& operator++() noexcept {
        ++idx;
        return *this;
    }

    inline index_view_iterator& operator--() noexcept {
        --idx;
        return *this;
    }

    inline index_view_iterator operator++(int) noexcept {
        auto tmp = *this;
        ++idx;
        return tmp;
    }

    inline index_view_iterator operator--(int) noexcept {
        auto tmp = *this;
        --idx;
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

    template <Iterable T>
    C operator()(const T& c) {
        C r;
        std::transform(std::begin(c), std::end(c), std::back_inserter(r), functor);
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

template <typename T, typename F> requires IsAdapter<F>
inline auto operator/(const T& v, F f) {
    return f(v);
}

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

/**
 * @brief Reads file line by line
 *
 * @param path - a path to the file
 *
 * @return lines of the file or nullopt if the file cannot be opened
 */
[[nodiscard]]
inline optional<vector<string>> read_lines(const string& path) {
    vector<string> result;

    std::ifstream ifs(path, std::ios::in);

    if (!ifs.is_open())
        return nullopt;

    while (!ifs.eof()) {
        string line;
        std::getline(ifs, line);
        result.emplace_back(std::move(line));
    }

    return result;
}

/**
 * @brief Reads file into string
 *
 * @param file_path - a path to the file
 *
 * @return the file content or nullopt if the file cannot be opened
 */
[[nodiscard]]
inline optional<string> read_file(const string& file_path) {
    std::ifstream ifs(file_path, std::ios_base::binary | std::ios_base::in);

    if (!ifs.is_open())
        return nullopt;

    ifs.seekg(0, std::ios_base::end);
    auto size = ifs.tellg();
    ifs.seekg(0, std::ios_base::beg);

    std::string str;
    str.resize(static_cast<size_t>(size));
    ifs.read(str.data(), size);

    return str;
}

/**
 * @brief Reads file into byte vector
 *
 * @param file_path - a path to the file
 *
 * @return the file content or nullopt if the file cannot be opened
 */
[[nodiscard]]
inline optional<vector<byte>> read_binary_file(const string& file_path) {
    std::ifstream ifs(file_path, std::ios_base::binary | std::ios_base::in);

    if (!ifs.is_open())
        return nullopt;

    ifs.seekg(0, std::ios_base::end);
    auto size = ifs.tellg();
    ifs.seekg(0, std::ios_base::beg);

    vector<byte> result(static_cast<size_t>(size));
    ifs.read(reinterpret_cast<char*>(result.data()), size); // NOLINT

    return result;
}

/**
 * @brief Reads file into string
 *
 * @throw runtime_error if the file cannot be opened
 *
 * @param file_path - a path to the file
 *
 * @return the file content
 */
[[nodiscard]]
inline string read_file_unwrap(const string& file_path) {
    if (auto file = read_file(file_path))
        return *file;
    else
        throw std::runtime_error("Can't open file '" + file_path + "'");
}

/**
 * @brief Reads file into byte vector
 *
 * @throw runtime_error if the file cannot be opened
 *
 * @param file_path - a path to the file
 *
 * @return the file content
 */
[[nodiscard]]
inline vector<byte> read_binary_file_unwrap(const string& file_path) {
    if (auto file = read_binary_file(file_path))
        return *file;
    else
        throw std::runtime_error("Can't open file '" + file_path + "'");
}

/**
 * @brief Writes a file from string
 *
 * @param file_path - path to new file
 * @param data - the file content
 * @param append - if true then appends data to the end of the file
 *
 * @return true if write successful, false otherwise
 */
[[nodiscard]]
inline bool write_file(const string& file_path, string_view data, bool append = false) {
    auto flags = std::ios_base::out;
    if (append)
        flags |= std::ios_base::app;

    std::ofstream ofs(file_path, flags);

    if (!ofs.is_open())
        return false;

    ofs.write(data.data(), static_cast<std::streamsize>(data.size()));

    return !ofs.bad();
}

/**
 * @brief Writes a file from byte span
 *
 * @param file_path - path to new file
 * @param data - the file content
 * @param append - if true then appends data to the end of the file
 *
 * @return true if write successful, false otherwise
 */
[[nodiscard]]
inline bool write_file(const string& file_path, span<byte> data, bool append = false) {
    auto flags = std::ios_base::out;
    if (append)
        flags |= std::ios_base::app;

    std::ofstream ofs(file_path, flags);

    if (!ofs.is_open())
        return false;

    ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size())); // NOLINT

    return !ofs.bad();
}

/**
 * @brief Writes a file from string
 *
 * @throw runtime_error if write failed
 *
 * @param file_path - path to new file
 * @param data - the file content
 * @param append - if true then appends data to the end of the file
 */
inline void write_file_unwrap(const string& file_path, string_view data, bool append = false) {
    if (!write_file(file_path, data, append))
        throw std::runtime_error("Can't write file '" + file_path + "'");
}

/**
 * @brief Writes a file from byte span
 *
 * @throw runtime_error if write failed
 *
 * @param file_path - path to new file
 * @param data - the file content
 * @param append - if true then appends data to the end of the file
 */
inline void write_file_unwrap(const string& file_path, span<byte> data, bool append = false) {
    if (!write_file(file_path, data, append))
        throw std::runtime_error("Can't write file '" + file_path + "'");
}

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

} // namespace core

namespace std
{
template <typename... Ts>
struct tuple_size<core::multi_view_tuple<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

template <size_t N, typename... Ts>
struct tuple_element<N, core::multi_view_tuple<Ts...>> : tuple_element<N, tuple<std::remove_reference_t<Ts>...>> {};
} // namespace std


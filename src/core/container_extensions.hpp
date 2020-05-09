#pragma once
#include <fstream>
#include <utility>
#include <ranges>
#include "platform_dependent.hpp"
#include "print.hpp"
#include "ston.hpp"


#define GENERIC_C_STRING(CHAR_T, STR) \
    []() constexpr { \
        static_assert(std::is_same_v<CHAR_T, char> || std::is_same_v<CHAR_T, char16_t> || std::is_same_v<CHAR_T, char32_t>, \
            "Invalid character type"); \
        if constexpr (std::is_same_v<CHAR_T, char>) \
            return STR; \
        else if constexpr (std::is_same_v<CHAR_T, char16_t>) \
            return u"#STR"; \
        else if constexpr (std::is_same_v<CHAR_T, char32_t>) \
            return U"#STR"; \
    }()


namespace core
{

    template <typename size_type>
    class index_view_iterator : public std::iterator<std::bidirectional_iterator_tag, size_type> {
    public:
        index_view_iterator(size_type init): idx(init) {}
        index_view_iterator() = default;

        inline index_view_iterator& operator++() {
            ++idx;
            return *this;
        }

        inline index_view_iterator& operator--() {
            --idx;
            return *this;
        }

        inline index_view_iterator operator++(int) {
            auto tmp = *this;
            ++idx;
            return tmp;
        }

        inline index_view_iterator operator--(int) {
            auto tmp = *this;
            --idx;
            return tmp;
        }

        inline bool operator==(const index_view_iterator& i) const {
            return idx == i.idx;
        }

        inline bool operator!=(const index_view_iterator& i) const {
            return !(*this == i);
        }

        inline const size_type& operator*() const {
            return idx;
        }

        inline size_type& operator*()  {
            return idx;
        }

    private:
        size_type idx = 0;
    };


    template <typename... Ts>
    struct multi_view_tuple : tuple<Ts...> {
        using tuple<Ts...>::tuple;

        template <typename... Tts, std::enable_if_t<(std::is_constructible_v<Ts, Tts&&> && ...), int> = 0>
        multi_view_tuple(tuple<Tts...>&& t) : tuple<Ts...>(std::move(t)) {}

        template <size_t N>
        auto& get() & {
            return std::get<N>(*this);
        }

        template <size_t N>
        auto& get() const& {
            return std::get<N>(*this);
        }

        template <size_t N>
        auto get() && {
            return std::get<N>(*this);
        }

        template <size_t N>
        auto get() const&& {
            return std::get<N>(*this);
        }
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
    inline auto multi_view_map_values(tuple<Ts...>& v, std::index_sequence<Idxs...>&&) {
        return multi_view_tuple<decltype(*std::declval<Ts>())...>(*std::get<Idxs>(v)...);
    }

    template <typename... Is>
    class double_view_iterator {
    public:
        using iterator_category = std::bidirectional_iterator_tag;

        double_view_iterator(Is... iiterators): iterators(iiterators...) {}

        inline double_view_iterator& operator++() {
            multi_view_increment(iterators, std::make_index_sequence<sizeof...(Is)>());
            return *this;
        }

        inline double_view_iterator& operator--() {
            multi_view_decrement(iterators, std::make_index_sequence<sizeof...(Is)>());
            return *this;
        }

        inline double_view_iterator operator++(int) {
            double_view_iterator tmp = *this;
            multi_view_increment(iterators, std::make_index_sequence<sizeof...(Is)>());
            return tmp;
        }

        inline double_view_iterator operator--(int) {
            double_view_iterator tmp = *this;
            multi_view_decrement(iterators, std::make_index_sequence<sizeof...(Is)>());
            return tmp;
        }

        inline bool operator==(const double_view_iterator& i) const {
            return std::get<0>(iterators) == std::get<0>(i.iterators);
        }

        inline bool operator!=(const double_view_iterator& i) const {
            return !(*this == i);
        }

        decltype(auto) operator*() {
            value.emplace(multi_view_map_values(iterators, std::make_index_sequence<sizeof...(Is)>()));
            return *value;
        }

    private:
        tuple<Is...> iterators;
        optional<multi_view_tuple<decltype(*std::declval<Is>())...>> value;
    };


    template <typename IterT>
    class iterator_view_proxy {
    public:
        iterator_view_proxy(IterT begin, IterT end): _begin(begin), _end(end) {}

        IterT begin() const {
            return _begin;
        }

        IterT end() const {
            return _end;
        }

    private:
        IterT _begin;
        IterT _end;
    };

    template <typename IterT>
    auto index_view(IterT begin, IterT end) {
        return iterator_view_proxy(index_view_iterator<size_t>(), index_view_iterator(static_cast<size_t>(end - begin)));
    }

    template <typename IterT1, typename IterT2>
    auto zip_view(IterT1 begin1, IterT1 end1, IterT2 begin2, IterT2 end2) {
        return iterator_view_proxy(
                double_view_iterator(begin1, begin2),
                double_view_iterator(end1,   end2));
    }

    template <Iterable T>
    auto skip_view(T&& container, size_t start) {
        using difference_type = decltype(container.begin() - container.begin());
        return iterator_view_proxy(container.begin() + static_cast<difference_type>(start), container.end());
    }

    template <typename I>
    auto value_index_view(I begin, I end) {
        return zip_view(begin, end, index_view_iterator<size_t>(0), index_view_iterator(static_cast<size_t>(end - begin)));
    }

    template <Iterable T>
    auto index_view(T&& container) {
        return index_view(container.begin(), container.end());
    }

    template <Iterable T1, Iterable T2>
    auto zip_view(T1&& container1, T2&& container2) {
        return iterator_view_proxy(
            double_view_iterator(container1.begin(), container2.begin()),
            double_view_iterator(container1.end(),   container2.end()));
    }

    template <Iterable T>
    auto value_index_view(T&& container) {
        return zip_view(std::forward<T>(container), index_view(container.begin(), container.end()));
    }


    template <typename ContainerT, typename T> requires Iterable<ContainerT>
    inline auto _fold_operator(const ContainerT& t, const T& delimiter) {
        using ItemT = std::remove_const_t<std::decay_t<decltype(t[0])>>;

        if (std::empty(t))
            return ItemT();
        else {
            ItemT result;
            auto inserter = std::back_inserter(result);
            auto begin    = std::begin(t);
            auto end      = std::end(t);

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
    struct count_if {
        using chain_functor = void;

        count_if(F callback): functor(std::move(callback)) {}

        template <Iterable T>
        bool operator()(const T& c) {
            return std::any_of(std::begin(c), std::end(c), functor);
        }

        F functor;
    };

    template <typename F>
    struct any_of {
        using chain_functor = void;

        any_of(F callback): functor(std::move(callback)) {}

        template <Iterable T>
        bool operator()(const T& c) {
            return std::any_of(std::begin(c), std::end(c), functor);
        }

        F functor;
    };

    template <typename F>
    struct all_of {
        using chain_functor = void;

        all_of(F callback): functor(std::move(callback)) {}

        template <Iterable T>
        bool operator()(const T& c) {
            return std::all_of(std::begin(c), std::end(c), functor);
        }

        F functor;
    };

    template <typename F>
    struct find_if {
        using chain_functor = void;

        find_if(F callback): functor(std::move(callback)) {}

        template <Iterable TT>
        auto operator()(const TT& c) {
            return std::find_if(std::begin(c), std::end(c), functor);
        }

        F functor;
    };

    template <typename C, typename F>
    struct transform_t {
        using chain_functor = void;

        transform_t(F callback): functor(std::move(callback)) {}

        template <Iterable T>
        C operator()(const T& c) {
            C r;
            std::transform(std::begin(c), std::end(c), std::back_inserter(r), functor);
            return r;
        }

        F functor;
    };

    template <typename C, typename F>
    transform_t<C, F> transform(F functor) {
        return transform_t<C, F>(functor);
    }


    template <typename F = std::less<>>
    struct is_sorted {
        using chain_functor = void;

        is_sorted(F callback = F()): functor(std::move(callback)) {}

        template <Iterable T>
        bool operator()(const T& c) {
            return std::is_sorted(std::begin(c), std::end(c), functor);
        }

        F functor;
    };

    template <typename F = std::minus<>>
    struct adjacent_difference {
        using chain_functor = void;

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

    template <typename T, typename F>
    struct reduce {
        using chain_functor = void;

        template <Iterable TT>
        auto operator()(const TT& c) {
            return std::reduce(std::begin(c), std::end(c), result, functor);
        }

        reduce(T initial, F callback): result(initial), functor(callback) {}

    private:
        T result;
        F functor;
    };


    template <typename T>
    struct fold {
        using chain_functor = void;

        template <typename ContainerT>
        auto operator()(ContainerT&& c) {
            if constexpr (Iterable<T>)
                return _fold_operator(std::forward<ContainerT>(c), delimiter);
            else {
                std::decay_t<decltype(c[0])> d = { delimiter };
                return _fold_operator(std::forward<ContainerT>(c), d);
            }
        }

        fold(T delim): delimiter(std::move(delim)) {}

        T delimiter;
    };


    template <typename T>
    struct fold <const T*> {
        using chain_functor = void;

        template <typename ContainerT>
        auto operator()(const ContainerT& c) {
            return _fold_operator(c, std::basic_string_view(delimiter));
        }

        explicit fold(const T* delim): delimiter(delim) {}

        const T* delimiter;
    };

    template <typename NumberT = int>
    struct to_number {
        using chain_functor = void;

        template <typename T>
        NumberT operator()(const T& str) {
            return ston<NumberT>(str);
        }
    };

    struct remove_trailing_whitespaces {
        using chain_functor = void;

        template <typename T>
        T operator()(const T& str) {
            if (str.empty())
                return {};

            typename T::size_type start = 0;
            typename T::size_type end   = str.size() - 1;

            for (; start < str.size() && (str[start] == ' ' || str[start] == '\t'); ++start);
            for (; start < end        && (str[end]   == ' ' || str[end]   == '\t'); --end);

            return str.substr(start, end - start + 1);
        }

        string_view operator()(const char* str) {
            return operator()(string_view(str));
        }
    };

    template <typename CharT, typename StrT = std::basic_string<CharT>>
    struct split {
        using chain_functor = void;

        template<typename T> requires StringLike<T>
        vector<StrT> operator()(const T& str) {
            vector<StrT> vec;
            std::basic_string_view<CharT> data = str;
            size_t start = 0;

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

        split(const vector<CharT>& delimiters, bool should_create_empty_strings = false) :
                delims(delimiters), create_empty_strings(should_create_empty_strings) {}

        split(std::initializer_list<CharT> delimiters, bool should_create_empty_strings = false) :
                delims(delimiters), create_empty_strings(should_create_empty_strings) {}

        split(CharT delimiter, bool should_create_empty_strings = false) :
                delims(1, delimiter), create_empty_strings(should_create_empty_strings) {}

        vector<CharT> delims;
        bool create_empty_strings = false;
    };


    template<typename CharT>
    struct split_view : public split<CharT, std::basic_string_view<CharT>> {
        explicit
        split_view(const vector<CharT>& delimiters, bool should_create_empty_strings = false) :
                split<CharT, std::basic_string_view<CharT>>(delimiters, should_create_empty_strings) {}

        split_view(std::initializer_list<CharT> delimiters, bool should_create_empty_strings = false) :
                split<CharT, std::basic_string_view<CharT>>(delimiters, should_create_empty_strings) {}

        explicit
        split_view(CharT delimiter, bool should_create_empty_strings = false) :
                split<CharT, std::basic_string_view<CharT>>(delimiter, should_create_empty_strings) {}
    };


    // Chains
    template<typename T, typename F> requires ChainFunctor<F>
    inline auto operator/(const T& v, F f) {
        return f(v);
    }


    template <typename CharT>
    inline std::basic_string<CharT> path_eval(std::basic_string_view<CharT> src) {
        if (src.empty())
            return std::basic_string<CharT>();

        auto has_slash_prefix  = src.front() == static_cast<CharT>('/');
        auto has_slash_postfix = src.back()  == static_cast<CharT>('/');

        auto src_begin = has_slash_prefix  ? src.begin() + 1 : src.begin();
        auto src_end   = has_slash_postfix ? src.end()   - 1 : src.end();

        if (src_end < src_begin)
            return std::basic_string<CharT>(GENERIC_C_STRING(CharT, "/"));

        auto splits = std::basic_string_view(src_begin,
                      static_cast<typename std::basic_string_view<CharT>::size_type>(src_end - src_begin)) /
                      split_view(static_cast<CharT>('/'),
                      true);
        auto last_inner_double_slash = std::find_if(splits.rbegin(), splits.rend(), [](const auto& s) { return s.empty(); });

        auto begin = (last_inner_double_slash != splits.rend()) ?
                     has_slash_prefix = true, splits.begin() + std::distance(last_inner_double_slash, splits.rend()) : // -1 was missed because we get token after "/"
                     splits.begin();

        std::vector<std::basic_string_view<CharT>> result;

        for (; begin != splits.end(); ++begin) {
            if (*begin == GENERIC_C_STRING(CharT, "..")) {
                if (!result.empty() && result.back() != GENERIC_C_STRING(CharT, "..")) {
                    result.pop_back();

                    if (result.empty())
                        has_slash_prefix = true;
                } else {
                    result.emplace_back(*begin);
                }
            } else if (*begin == GENERIC_C_STRING(CharT, "~")) {
                result.clear();
                result.emplace_back(*begin);
            } else if (*begin != GENERIC_C_STRING(CharT, ".")) {
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
        } else if (has_slash_prefix) {
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

    inline string path_eval(string_view str) {
        return path_eval<char>(str);
    }

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

    inline optional<vector<byte>> read_binary_file(const string& file_path) {
        std::ifstream ifs(file_path, std::ios_base::binary | std::ios_base::in);

        if (!ifs.is_open())
            return nullopt;

        ifs.seekg(0, std::ios_base::end);
        auto size = ifs.tellg();
        ifs.seekg(0, std::ios_base::beg);

        vector<byte> result(static_cast<size_t>(size));
        ifs.read(reinterpret_cast<char*>(result.data()), size);

        return result;
    }

    inline string read_file_unwrap(const string& file_path) {
        if (auto file = read_file(file_path))
            return *file;
        else
            throw std::runtime_error("Can't open file '" + file_path + "'");
    }

    inline vector<byte> read_binary_file_unwrap(const string& file_path) {
        if (auto file = read_binary_file(file_path))
            return *file;
        else
            throw std::runtime_error("Can't open file '" + file_path + "'");
    }

    /**
     * Returns true if file has been written
     */
    inline bool write_file(const string& file_path, string_view data) {
        std::ofstream ofs(file_path, std::ios_base::out);

        if (!ofs.is_open())
            return false;

        ofs.write(data.data(), static_cast<std::streamsize>(data.size()));
        return true;
    }

    /**
     * Returns true if file has been written
     */
    inline bool write_file(const string& file_path, span<byte> data) {
        std::ofstream ofs(file_path, std::ios_base::out);

        if (!ofs.is_open())
            return false;

        ofs.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        return true;
    }

    inline void write_file_unwrap(const string& file_path, string_view data) {
        if (!write_file(file_path, data))
            throw std::runtime_error("Can't write file '" + file_path + "'");
    }

    inline void write_file_unwrap(const string& file_path, span<byte> data) {
        if (!write_file(file_path, data))
            throw std::runtime_error("Can't write file '" + file_path + "'");
    }

    inline bool case_insensitive_match(string_view str1, string_view str2) {
        auto a = std::string(str1);
        auto b = std::string(str2);

        std::transform(a.begin(), a.end(), a.begin(), ::tolower);
        std::transform(b.begin(), b.end(), b.begin(), ::tolower);

        return a == b;
    }

    template <typename CharT>
    inline std::basic_string<CharT> operator/ (std::basic_string_view<CharT> path1, std::basic_string_view<CharT> path2) {
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


    inline core::string operator/ (core::string_view path1, core::string_view path2) {
        return operator/<char>(path1, path2);
    }
} // namespace core

namespace std {
    template <typename... Ts>
    struct tuple_size<core::multi_view_tuple<Ts...>> : std::integral_constant<size_t, sizeof...(Ts)> {};

    template <size_t N, typename... Ts>
    struct tuple_element<N, core::multi_view_tuple<Ts...>> : tuple_element<N, tuple<remove_reference_t<Ts>...>> {};
}

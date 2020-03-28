#pragma once
#include <fstream>
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
    template <typename ContainerT, typename T> requires Iterable<ContainerT>
    auto _fold_operator(const ContainerT& t, const T& delimiter) {
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
    auto operator/(const T& v, F f) {
        return f(v);
    }


    template <typename CharT>
    std::basic_string<CharT> path_eval(std::basic_string_view<CharT> src) {
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

    string path_eval(string_view str) {
        return path_eval<char>(str);
    }

    optional<vector<string>> read_lines(const string& path) {
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

    optional<string> read_file(const string& file_path) {
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


    template <typename CharT>
    std::basic_string<CharT> operator/ (std::basic_string_view<CharT> path1, std::basic_string_view<CharT> path2) {
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


    core::string operator/ (core::string_view path1, core::string_view path2) {
        return operator/<char>(path1, path2);
    }
} // namespace core


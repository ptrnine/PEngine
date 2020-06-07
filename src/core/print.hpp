#pragma once

#include "types.hpp"
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>
#include <type_traits>

namespace core
{
template <typename T>
struct magic_printer;

template <typename T>
concept PrintStandard = requires(T&& v) {
    std::cout << v;
};

static constexpr size_t max_tuple_size = 32;
template <typename T>
concept PrintTupleLike = std::tuple_size<std::remove_reference_t<T>>::value <
                         max_tuple_size; // prevent std::array<T, 10000000> compile-time jokes

template <typename T>
concept PrintIterable = (!PrintStandard<T>)&&(!PrintTupleLike<T>)&&requires(T&& t) {
    std::begin(t);
    std::end(t);
};

template <typename T>
concept PrintMember = requires(const T& v) {
    v.print(std::cout);
};

template <typename T>
concept EnumClass = std::is_enum_v<T>;

template <typename T>
concept TupleConvertible = !PrintMember<T> && requires(const T& v) {
    v.to_tuple();
};

template <typename T>
concept MagicPrinterCheck = requires(std::ostream& os, const T& v) {
    magic_printer<T>()(os, v);
};

template <typename T>
concept Printable = PrintIterable<T> || PrintMember<T> || PrintTupleLike<T> || PrintStandard<T> || MagicPrinterCheck<T>;

template <typename T1, typename T2, typename... Ts>
void magic_print(std::ostream& os, T1&& v1, T2&& v2, Ts&&... args);

template <TupleConvertible T>
void magic_print(std::ostream& os, const T& v);

template <MagicPrinterCheck T>
void magic_print(std::ostream& os, const T& v);

inline void magic_print(std::ostream& os, std::byte b) {
    os << "0x"
       << "0123456789abcdef"[static_cast<uint8_t>(b) >> 4]    // NOLINT
       << "0123456789abcdef"[static_cast<uint8_t>(b) & 0x0f]; // NOLINT
}

template <EnumClass T>
void magic_print(std::ostream& os, T v) {
    os << std::underlying_type_t<T>(v);
}

inline void magic_print(std::ostream& os, bool v) {
    os << (v ? "true" : "false");
}

inline void magic_print(std::ostream& os, const vector<bool>::reference v) {
    os << (v ? "true" : "false");
}

template <PrintStandard T>
void magic_print(std::ostream& os, T&& val) {
    os << std::forward<T>(val);
}

template <PrintIterable T>
void magic_print(std::ostream& os, const T& val) {
    auto begin = std::begin(val);
    auto end   = std::end(val);

    if (begin == end)
        os << "{}";
    else {
        magic_print(os, "{ ", *begin++);
        for (; begin != end; ++begin) magic_print(os, ", ", *begin);
        magic_print(os, " }");
    }
}

template <PrintMember T>
void magic_print(std::ostream& os, const T& val) {
    val.print(os);
}

template <size_t idx = 1, PrintTupleLike T>
void static_cortage_print_impl(std::ostream& os, const T& val) {
    using std::get;
    if constexpr (std::tuple_size_v<T> != idx) {
        magic_print(os, ", ", get<idx>(val));
        static_cortage_print_impl<idx + 1>(os, val);
    }
    else {
        magic_print(os, " }");
    }
}

template <PrintTupleLike T>
void magic_print(std::ostream& os, const T& val) {
    using std::get;
    if constexpr (std::tuple_size_v<T> == 0)
        os << "{}";
    else {
        magic_print(os, "{ ", get<0>(val));
        static_cortage_print_impl(os, val);
    }
}

template <typename T> requires SpecializationOf<T, optional>
void magic_print(std::ostream& os, const T& val) {
    if (val)
        magic_print(os, *val);
    else
        os << "nullopt";
}

template <typename T1, typename T2, typename... Ts>
void magic_print(std::ostream& os, T1&& v1, T2&& v2, Ts&&... args) {
    magic_print(os, std::forward<T1>(v1));
    magic_print(os, std::forward<T2>(v2));
    ((magic_print(os, std::forward<Ts>(args))), ...);
}

template <TupleConvertible T>
void magic_print(std::ostream& os, const T& v) {
    magic_print(os, v.to_tuple());
}

template <MagicPrinterCheck T>
void magic_print(std::ostream& os, const T& v) {
    magic_printer<T>()(os, v);
}

inline void fmt_proc(std::ostream& os, std::string_view str) {
    if (str.front() == '.')
        os << std::setprecision(std::stoi(std::string(str.substr(1))));
}

template <size_t idx = 0, typename IterT, typename... ArgsT>
size_t print_iter(std::ostream& os, IterT begin, IterT end, ArgsT&&... args) {
    bool   on_bracket = false;
    size_t rc         = idx;
    string fmt;

    for (; begin != end; ++begin) {
        if (!on_bracket) {
            if (*begin == '{')
                on_bracket = true;
            else if (*begin == '}')
                throw std::invalid_argument("Invalid format string");
            else
                os.put(*begin);
        }
        else {
            if (*begin == '}') {
                on_bracket = false;

                if constexpr (idx < sizeof...(ArgsT)) {
                    using std::get;

                    // Todo: do something with fmt
                    std::ios saved(nullptr);
                    saved.copyfmt(os);

                    if (!fmt.empty())
                        fmt_proc(os, fmt);

                    magic_print(os, get<idx>(std::forward_as_tuple(forward<ArgsT>(args)...)));

                    os.copyfmt(saved);

                    rc = print_iter<idx + 1>(os, begin + 1, end, forward<ArgsT>(args)...);
                }
                else {
                    rc += 1;
                }

                return rc;
            }
            else if (*begin == '{')
                throw std::invalid_argument("Invalid format string");
            else
                fmt.push_back(*begin);
        }
    }

    return rc;
}

static constexpr int print_float_precision = 7;

template <typename... ArgsT>
void print_base(std::ostream& os, string_view fmt, ArgsT&&... args) {
    os << std::setprecision(print_float_precision);
    size_t count = print_iter(os, std::begin(fmt), std::end(fmt), forward<ArgsT>(args)...);

    if (count != sizeof...(ArgsT))
        throw std::invalid_argument("Invalid format string: wrong arguments count");
}

template <typename... ArgsT>
void print(string_view fmt, ArgsT&&... args) {
    print_base(std::cout, fmt, forward<ArgsT>(args)...);
}

template <typename... ArgsT>
void printline(string_view fmt, ArgsT&&... args) {
    print_base(std::cout, fmt, forward<ArgsT>(args)...);
    std::cout << std::endl;
}

template <typename... ArgsT>
string format(string_view fmt, ArgsT&&... args) {
    std::stringstream ss;
    ss << std::setprecision(print_float_precision);
    print_base(ss, fmt, forward<ArgsT>(args)...);
    return ss.str();
}

} // namespace core

#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>
#include <type_traits>
#include "types.hpp"

namespace core
{
    namespace {
        template <typename T>
        concept PrintStandard = requires (T&& v) { std::cout << v; };

        template <typename T>
        concept PrintStaticCortege = std::tuple_size<std::remove_reference_t<T>>::value < 32; // prevent std::array<T, 10000000> compile-time jokes

        template <typename T>
        concept PrintIterable = (!PrintStandard<T>) && (!PrintStaticCortege<T>) && requires (T&& t) { std::begin(t); std::end(t); };

        template <typename T>
        concept PrintMember = requires (T&& v) { v.print(std::cout); };

        template <typename T>
        concept Printable = PrintIterable<T> || PrintMember<T> || PrintStaticCortege<T> || PrintStandard<T>;

        template <typename T1, typename T2, typename... Ts>
        void magic_print(std::ostream& os, T1&& v1, T2&& v2, Ts&&... args);

        void magic_print(std::ostream& os, std::byte b) {
            os << "0x" << "0123456789abcdef"[static_cast<uint8_t>(b) >> 4]
               << "0123456789abcdef"[static_cast<uint8_t>(b) & 0x0f];
        }

        void magic_print(std::ostream& os, bool v) {
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
                for (; begin != end; ++begin)
                    magic_print(os, ", ", *begin);
                magic_print(os, " }");
            }
        }

        template <PrintMember T>
        void magic_print(std::ostream& os, const T& val) {
            val.print(os);
        }

        template <size_t idx = 1, PrintStaticCortege T>
        void static_cortage_print_impl(std::ostream& os, const T& val) {
            using std::get;
            if constexpr (std::tuple_size_v<T> != idx) {
                magic_print(os, ", ", get<idx>(val));
                static_cortage_print_impl<idx + 1>(os, val);
            } else {
                magic_print(os, " }");
            }
        }

        template <PrintStaticCortege T>
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

        template <size_t idx = 0, typename IterT, typename... ArgsT>
        size_t print_iter(std::ostream& os, IterT begin, IterT end, ArgsT&&... args) {
            bool on_bracket = false;
            size_t rc = idx;
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
                            magic_print(os, get<idx>(std::forward_as_tuple(forward<ArgsT>(args)...)));
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
    } // anonymous namespace


    template<typename... ArgsT>
    void print_base(std::ostream& os, string_view fmt, ArgsT&& ... args) {
        os << std::setprecision(16);
        size_t count = print_iter(os, std::begin(fmt), std::end(fmt), forward<ArgsT>(args)...);

        if (count != sizeof...(ArgsT))
            throw std::invalid_argument("Invalid format string: wrong arguments count");
    }

    template<typename... ArgsT>
    void print(string_view fmt, ArgsT&& ... args) {
        print_base(std::cout, fmt, forward<ArgsT>(args)...);
    }

    template<typename... ArgsT>
    string format(string_view fmt, ArgsT&&... args) {
        std::stringstream ss;
        ss << std::setprecision(16);
        print_base(ss, fmt, forward<ArgsT>(args)...);
        return ss.str();
    }

} // namespace core

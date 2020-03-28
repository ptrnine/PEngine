#pragma once

#include "types.hpp"

namespace core {
    template <typename T>
    inline T ston(string_view str) {
        static_assert(Integral<T> || AnyOfType<T, double, float>, "T is not a number");

        if constexpr (Integral<T>) {
            if constexpr (Unsigned<T>)
                return static_cast<T>(std::stoull(string(str)));
            else
                return static_cast<T>(std::stoll(string(str)));
        }
        else if constexpr (std::is_same_v<float, T>) {
            return std::stof(string(str));
        }
        else if constexpr (std::is_same_v<double, T>) {
            return std::stod(string(str));
        }
    }
}
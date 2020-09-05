#pragma once

#include "types.hpp"
#include "vec.hpp"

namespace core {
    template <FloatingPoint T>
    T lerp(T v0, T v1, T t) {
        return v0 * (1 - t) + v1 * t;
    }

    template <FloatingPoint T, size_t S>
    core::vec<T, S> lerp(const core::vec<T, S>& v0, const core::vec<T, S>& v1, T t) {
        return v0 * (1 - t) + v1 * t;
    }

    template <FloatingPoint T, size_t S>
    core::vec<T, S> pow(const core::vec<T, S>& vec, T power) {
        return vec_map(vec, [=](auto v) { return std::pow(v, power); } );
    }

    template <FloatingPoint T, size_t S>
    core::vec<T, S> gamma_correction(const core::vec<T, S>& vec, T gamma) {
        return pow(vec, gamma);
    }

    template <FloatingPoint T>
    T inverse_lerp(T x1, T x2, T value) {
        return (value - x1) / (x2 - x1);
    }

    template <FloatingPoint T>
    T unit_clamp(T val) {
        return std::clamp(val, T(0), T(1));
    }

    template <typename T>
    T constraint(T number, T min, T max) {
        T distance = max - min;
        return std::fmod(distance + std::fmod(number - min, distance), distance) + min;
    }

    namespace angle {
        /// Constraint angle [0 ; 2PI]
        template <FloatingPoint T>
        T constraint_2pi(T angle) {
            angle = std::fmod(angle, T(2) * T(M_PIf64));
            return angle < 0 ? angle + T(2) * T(M_PIf64) : angle;
        }
        template <FloatingPoint T, size_t S>
        vec<T, S> constraint_2pi(const vec<T, S>& angles) {
            return vec_map(angles, [](T v) { return constraint_2pi(v); });
        }

        /// Constraint angle [-PI ; +PI]
        template <FloatingPoint T>
        T constraint_pi(T angle) {
            angle = std::fmod(angle + T(M_PIf64), T(2) * T(M_PIf64));
            return angle < 0 ? angle + T(M_PIf64) : angle - T(M_PIf64);
        }
        template <FloatingPoint T, size_t S>
        vec<T, S> constraint_pi(const vec<T, S>& angles) {
            return vec_map(angles, [](T v) { return constraint_pi(v); });
        }


        template <typename T>
        T add_2pi(T alpha, T betha) {
            return constraint_2pi(alpha + betha);
        }

        template <typename T>
        T sub_2pi(T alpha, T betha) {
            return constraint_2pi(alpha - betha);
        }

        template <FloatingPoint T>
        T radian(T degree) {
            return degree * (T(M_PIf64) / T(180));
        }
        template <FloatingPoint T, size_t S>
        vec<T, S> radian(const vec<T, S>& degrees) {
            return vec_map(degrees, [](T v) { return radian(v); });
        }

        template <FloatingPoint T>
        T degree(T radian) {
            return radian * (T(180) / T(M_PIf64));
        }
        template <FloatingPoint T, size_t S>
        vec<T, S> degree(const vec<T, S>& radians) {
            return vec_map(radians, [](T v) { return degree(v); });
        }
    } // namespace angle
} // namespace core


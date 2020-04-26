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

    template <FloatingPoint T>
    T inverse_lerp(T x1, T x2, T value) {
        return (value - x1) / (x2 - x1);
    }

    template <FloatingPoint T>
    T unit_clamp(T val) {
        return std::clamp(val, T(0), T(1));
    }

    namespace angle {
        /// Constraint angle [0 ; 2PI]
        template <FloatingPoint T>
        T constraint_2pi(T angle) {
            angle = std::fmod(angle + T(M_PIf64), T(2) * T(M_PIf64));
            return angle < 0 ? angle + T(M_PIf64) : angle - T(M_PIf64);
        }

        /// Constraint angle [-PI ; +PI]
        template <FloatingPoint T>
        T constraint_pi(T angle) {
            angle = std::fmod(angle + T(M_PIf64), T(2) * T(M_PIf64));
            return angle < 0 ? angle + T(M_PIf64) : angle - T(M_PIf64);
        }
        template <FloatingPoint T>
        T add_2pi(T alpha, T betha) {
            return constraint_2pi(alpha + betha);
        }

        template <FloatingPoint T>
        T sub_2pi(T alpha, T betha) {
            return constraint_2pi(alpha - betha);
        }

        template <FloatingPoint T>
        T radian(T degree) {
            return degree * (T(M_PIf64) / T(180));
        }

        template <FloatingPoint T>
        T degree(T radian) {
            return radian * (T(180) / T(M_PIf64));
        }
    }
}
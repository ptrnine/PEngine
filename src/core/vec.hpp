#pragma once
#include <tuple>
#include <array>
#include <cmath>
#include <cstring>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "vec_macro_gen.hpp"

namespace core {
    /*
     * Floating point op
     */
    inline bool approx_equal(float a, float b, float epsilon) {
        return fabsf(a - b) <= ((fabsf(a) < fabsf(b) ? fabsf(b) : fabsf(a)) * epsilon);
    }

    inline bool approx_equal(double a, double b, double epsilon) {
        return fabs(a - b) <= ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
    }

    inline bool essentially_equal(float a, float b, float epsilon) {
        return fabsf(a - b) <= ((fabsf(a) > fabsf(b) ? fabsf(b) : fabsf(a)) * epsilon);
    }

    inline bool essentially_equal(double a, double b, double epsilon) {
        return fabs(a - b) <= ((fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * epsilon);
    }

    inline bool definitely_greater(float a, float b, float epsilon) {
        return (a - b) > ((fabsf(a) < fabsf(b) ? fabsf(b) : fabsf(a)) * epsilon);
    }

    inline bool definitely_greater(double a, double b, double epsilon) {
        return (a - b) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
    }

    inline bool definitely_less(float a, float b, float epsilon) {
        return (b - a) > ((fabsf(a) < fabsf(b) ? fabsf(b) : fabsf(a)) * epsilon);
    }

    inline bool definitely_less(double a, double b, double epsilon) {
        return (b - a) > ((fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
    }

    /*
     * Fast inverse square root
     */

    template <typename T>
    struct fisr_magic_constant;

    template <>
    struct fisr_magic_constant<float> : public std::integral_constant<uint32_t, 0x5f3759df> {};

    template <>
    struct fisr_magic_constant<double> : public std::integral_constant<uint64_t, 0x5fe6eb50c7b537a9> {};

    template <typename T>
    struct float_to_uint_type;

    template <>
    struct float_to_uint_type<float> { using type = uint32_t; };

    template <>
    struct float_to_uint_type<double> { using type = uint64_t; };

    template <typename T>
    using float_to_uint_type_t = typename float_to_uint_type<T>::type;

    template <typename T, size_t Step>
    inline T fisr_loop(T y, T x2) {
        if constexpr (Step == 0)
            return y;
        else
            return fisr_loop<T, Step - 1>(y * (T(1.5) - x2 * y * y), x2);
    }

    template <size_t Steps = 1, typename T>
    inline T fast_inverse_square_root(T value) {
        T x2 = value * T(0.5);
        T y  = value;
        float_to_uint_type_t<T> i;
        memcpy(&i, &y, sizeof(T));

        i = fisr_magic_constant<T>::value - (i >> 1);
        memcpy(&y, &i, sizeof(T));

        return fisr_loop<T, Steps>(y, x2);
    }


    /*
     * vector operations
     */


    template <typename A, typename B, size_t... Idxs>
    constexpr inline bool vec_integer_equal(
            const std::array<A, sizeof...(Idxs)>& a,
            const std::array<B, sizeof...(Idxs)>& b,
            std::index_sequence<Idxs...>&&)
    {
        return true && ((std::get<Idxs>(a) == std::get<Idxs>(b)) && ...);
    }

    template <typename A, size_t... Idxs>
    constexpr inline auto vec_float_essentially_equal(
            const std::array<A, sizeof...(Idxs)>& a,
            const std::array<A, sizeof...(Idxs)>& b,
            A epsilon,
            std::index_sequence<Idxs...>&&)
    {
        return true && (essentially_equal(std::get<Idxs>(a), std::get<Idxs>(b), epsilon) && ...);
    }

    template <typename A, size_t... Idxs>
    constexpr inline auto vec_float_approx_equal(
            const std::array<A, sizeof...(Idxs)>& a,
            const std::array<A, sizeof...(Idxs)>& b,
            A epsilon,
            std::index_sequence<Idxs...>&&)
    {
        return true && (approx_equal(std::get<Idxs>(a), std::get<Idxs>(b), epsilon) && ...);
    }

    template <typename A, typename B, size_t... Idxs>
    constexpr inline auto vec_add(const std::array<A, sizeof...(Idxs)>& a,
                                  const std::array<B, sizeof...(Idxs)>& b,
                                  std::index_sequence<Idxs...>&&)
    {
        return std::array{(std::get<Idxs>(a) + std::get<Idxs>(b))...};
    }

    template <typename A, typename B, size_t... Idxs>
    constexpr inline auto vec_sub(const std::array<A, sizeof...(Idxs)>& a,
                                  const std::array<B, sizeof...(Idxs)>& b,
                                  std::index_sequence<Idxs...>&&)
    {
        return std::array{(std::get<Idxs>(a) - std::get<Idxs>(b))...};
    }

    template <typename A, typename B, size_t... Idxs>
    constexpr inline void vec_fetch_add(std::array<A, sizeof...(Idxs)>& a,
                                        const std::array<B, sizeof...(Idxs)>& b,
                                        std::index_sequence<Idxs...>&&)
    {
        ((std::get<Idxs>(a) += std::get<Idxs>(b)), ...);
    }

    template <typename A, typename B, size_t... Idxs>
    constexpr inline void vec_fetch_sub(std::array<A, sizeof...(Idxs)>& a,
                                        const std::array<B, sizeof...(Idxs)>& b,
                                        std::index_sequence<Idxs...>&&)
    {
        ((std::get<Idxs>(a) -= std::get<Idxs>(b)), ...);
    }

    template <typename A, size_t... Idxs>
    inline A vec_magnitude_2(const std::array<A, sizeof...(Idxs)>& a, std::index_sequence<Idxs...>&&) {
        return ((std::get<Idxs>(a) * std::get<Idxs>(a)) + ...);
    }

    template <typename A, size_t Size>
    inline A vec_magnitude(const std::array<A, Size>& a) {
        return sqrt(vec_magnitude_2(a, std::make_index_sequence<Size>()));
    }

    template <size_t Steps = 1, typename A, size_t Size>
    inline A vec_fast_inverse_magnitude(const std::array<A, Size>& a) {
        return fast_inverse_square_root(vec_magnitude_2(a, std::make_index_sequence<Size>()));
    }

    template <size_t Steps = 1, typename A, size_t Size>
    inline A vec_fast_magnitude(const std::array<A, Size>& a) {
        return 1 / vec_fast_inverse_magnitude<Steps>(a);
    }

    template <typename A, size_t... Idxs>
    inline std::array<A, sizeof...(Idxs)>
    vec_normalize(const std::array<A, sizeof...(Idxs)>& a, std::index_sequence<Idxs...>&&) {
        auto magnitude = vec_magnitude(a);
        return std::array{(std::get<Idxs>(a) / magnitude)...};
    }

    template <size_t Steps = 1, typename A, size_t... Idxs>
    inline std::array<A, sizeof...(Idxs)>
    vec_fast_normalize(const std::array<A, sizeof...(Idxs)>& a, std::index_sequence<Idxs...>&&) {
        auto inverse_magnitude = vec_fast_inverse_magnitude<Steps>(a);
        return std::array{(std::get<Idxs>(a) * inverse_magnitude)...};
    }

    template <typename A, size_t... Idxs>
    inline void vec_fetch_normalize(std::array<A, sizeof...(Idxs)>& a, std::index_sequence<Idxs...>&&) {
        auto magnitude = vec_magnitude(a);
        ((std::get<Idxs>(a) / magnitude), ...);
    }

    template <size_t Steps = 1, typename A, size_t... Idxs>
    inline void vec_fetch_fast_normalize(std::array<A, sizeof...(Idxs)>& a, std::index_sequence<Idxs...>&&) {
        auto inverse_magnitude = vec_fast_inverse_magnitude<Steps>(a);
        ((std::get<Idxs>(a) * inverse_magnitude), ...);
    }

    template <typename A, typename B, size_t... Idxs>
    constexpr inline auto
    vec_dot_product(const std::array<A, sizeof...(Idxs)>& a,
                    const std::array<B, sizeof...(Idxs)>& b,
                    std::index_sequence<Idxs...>&&)
    {
        return ((std::get<Idxs>(a) * std::get<Idxs>(b)) + ...);
    }

#define VECTOR_SCALAR_GEN_OP(NAME, OPERATOR) \
    template <typename A, typename N, size_t... Idxs> \
    constexpr inline auto \
    vec_scalar_##NAME(const std::array<A, sizeof...(Idxs)>& a, N scalar, std::index_sequence<Idxs...>&&) { \
        return std::array{(std::get<Idxs>(a) OPERATOR scalar)...}; \
    }

    VECTOR_SCALAR_GEN_OP(add, +)
    VECTOR_SCALAR_GEN_OP(sub, -)
    VECTOR_SCALAR_GEN_OP(mul, *)
    VECTOR_SCALAR_GEN_OP(div, /)
#undef VECTOR_SCALAR_GEN_OP


#define VECTOR_SCALAR_FETCH_GEN_OP(NAME, OPERATOR) \
    template <typename A, typename N, size_t... Idxs> \
    constexpr inline auto \
    vec_scalar_fetch_##NAME(std::array<A, sizeof...(Idxs)>& a, N scalar, std::index_sequence<Idxs...>&&) { \
        ((std::get<Idxs>(a) OPERATOR scalar), ...); \
    }

    VECTOR_SCALAR_FETCH_GEN_OP(add, +=)
    VECTOR_SCALAR_FETCH_GEN_OP(sub, -=)
    VECTOR_SCALAR_FETCH_GEN_OP(mul, *=)
    VECTOR_SCALAR_FETCH_GEN_OP(div, /=)
#undef VECTOR_SCALAR_FETCH_GEN_OP

    /*
     * Basic vector
     */

    template <typename T, size_t S, template <typename, size_t> class DerivedT>
    struct vec_base {
        std::array<T, S> v;

        template <size_t N>
        constexpr decltype(auto) get() const { return std::get<N>(v); }

        template <size_t N>
        constexpr decltype(auto) get() { return std::get<N>(v); }

        template <typename TT>
        constexpr auto operator+ (const vec_base<TT, S, DerivedT>& vec) const {
            return DerivedT{vec_add(v, vec.v, std::make_index_sequence<S>())};
        }

        template <typename TT>
        constexpr auto operator- (const vec_base<TT, S, DerivedT>& vec) const {
            return DerivedT{vec_sub(v, vec.v, std::make_index_sequence<S>())};
        }

        template <typename TT>
        constexpr DerivedT<T, S>& operator+= (const vec_base<TT, S, DerivedT>& vec) {
            vec_fetch_add(v, vec.v, std::make_index_sequence<S>());
            return static_cast<DerivedT<T, S>&>(*this);
        }

        template <typename TT>
        constexpr DerivedT<T, S>& operator-= (const vec_base<TT, S, DerivedT>& vec) {
            vec_fetch_sub(v, vec.v, std::make_index_sequence<S>());
            return static_cast<DerivedT<T, S>&>(*this);
        }

        template <typename N>
        constexpr auto operator+ (N n) const {
            return DerivedT{vec_scalar_add(this->v, n, std::make_index_sequence<S>())};
        }

        template <typename N>
        constexpr auto operator- (N n) const {
            return DerivedT{vec_scalar_sub(this->v, n, std::make_index_sequence<S>())};
        }

        template <typename N>
        constexpr auto operator* (N n) const {
            return DerivedT{vec_scalar_mul(this->v, n, std::make_index_sequence<S>())};
        }

        template <typename N>
        constexpr auto operator/ (N n) const {
            return DerivedT{vec_scalar_div(this->v, n, std::make_index_sequence<S>())};
        }

        template <typename N>
        constexpr DerivedT<T, S>& operator+= (N n) {
            vec_scalar_fetch_add(this->v, n, std::make_index_sequence<S>());
            return static_cast<DerivedT<T, S>&>(*this);
        }

        template <typename N>
        constexpr DerivedT<T, S>& operator-= (N n) {
            vec_scalar_fetch_sub(this->v, n, std::make_index_sequence<S>());
            return static_cast<DerivedT<T, S>&>(*this);
        }

        template <typename N>
        constexpr DerivedT<T, S>& operator*= (N n) {
            vec_scalar_fetch_mul(this->v, n, std::make_index_sequence<S>());
            return static_cast<DerivedT<T, S>&>(*this);
        }

        template <typename N>
        constexpr DerivedT<T, S>& operator/= (N n) {
            vec_scalar_fetch_div(this->v, n, std::make_index_sequence<S>());
            return static_cast<DerivedT<T, S>&>(*this);
        }

        constexpr T magnitude_2() const {
            return vec_magnitude_2(this->v, std::make_index_sequence<S>());
        }

        template <typename TT>
        constexpr auto dot(const vec_base<TT, S, DerivedT>& vec) const {
            return vec_dot_product(this->v, vec.v, std::make_index_sequence<S>());
        }
    };

    /*
     * Non-floating point specific
     */
    template <typename T, size_t S, template <typename, size_t> class DerivedT, typename Enable = void>
    struct vec_specific : vec_base<T, S, DerivedT> {
        template <typename TT>
        bool operator== (const vec_base<TT, S, DerivedT>& vec) const {
            return vec_integer_equal(this->v, vec.v, std::make_index_sequence<S>());
        }

        template <typename TT>
        bool operator!= (const vec_base<TT, S, DerivedT>& vec) const {
            return !(*this == vec);
        }
    };


    /*
     * Floating point specific
     */
    template <typename T, size_t S, template <typename, size_t> class DerivedT>
    struct vec_specific<T, S, DerivedT, std::enable_if_t<std::is_floating_point<T>::value>> : vec_base<T, S, DerivedT> {
        bool essentially_equal(const DerivedT<T, S>& vec, T epsilon) const {
            return vec_float_essentially_equal(this->v, vec.v, epsilon, std::make_index_sequence<S>());
        }

        bool approx_equal(const DerivedT<T, S>& vec, T epsilon) const {
            return vec_float_approx_equal(this->v, vec.v, epsilon, std::make_index_sequence<S>());
        }

        T magnitude() const {
            return vec_magnitude(this->v);
        }

        template <size_t Iterations = 1>
        T fast_magnitude() const {
            return vec_fast_magnitude<Iterations>(this->v);
        }

        template <size_t Iterations = 1>
        T fast_inverse_magnitude() const {
            return vec_fast_inverse_magnitude<Iterations>(this->v);
        }

        DerivedT<T, S> normalize() const {
            return DerivedT<T, S>{vec_normalize(this->v, std::make_index_sequence<S>())};
        }

        DerivedT<T, S>& make_normalize() const {
            vec_fetch_normalize(this->v, std::make_index_sequence<S>());
            return static_cast<DerivedT<T, S>&>(*this);
        }

        template <size_t Iterations = 1>
        DerivedT<T, S> fast_normalize() const {
            return DerivedT<T, S>{vec_fast_normalize<Iterations>(this->v, std::make_index_sequence<S>())};
        }

        template <size_t Iterations = 1>
        DerivedT<T, S>& make_fast_normalize() const {
            vec_fetch_fast_normalize<Iterations>(this->v, std::make_index_sequence<S>());
            return static_cast<DerivedT<T, S>&>(*this);
        }
    };


    /*
     * Vectors by dimmensions
     */

    template <typename T, size_t S, template <typename, size_t> class DerivedT>
    struct vec1_base : public vec_specific<T, S, DerivedT> {
        void x(T _x)       { this->template get<0>() = _x; }
        T    x()     const { return this->template get<0>(); }
    };

    template <typename T, size_t S, template <typename, size_t> class DerivedT>
    struct vec2_base : public vec1_base<T, S, DerivedT> {
        void y(T _y)       { this->template get<1>() = _y; }
        T    y()     const { return this->template get<1>(); }

        GEN_GET_2_FROM_VEC2(x, y)
        GEN_SET_2_FROM_VEC2(x, y)
    };

    template <typename T, size_t S, template <typename, size_t> class DerivedT>
    struct vec3_base : public vec2_base<T, S, DerivedT> {
        void z(T _z)       { this->template get<2>() = _z; }
        T    z()     const { return this->template get<2>(); }

        void r(T _r)       { this->x(_r); }
        T    r()     const { return this->x(); }

        void g(T _g)       { this->y(_g); }
        T    g()     const { return this->y(); }

        void b(T _b)       { this->z(_b); }
        T    b()     const { return this->z(); }

        GEN_GET_2_FROM_VEC3(x, y, z)
        GEN_GET_2_FROM_VEC3(r, g, b)
        GEN_GET_3_FROM_VEC3(x, y, z)
        GEN_GET_3_FROM_VEC3(r, g, b)

        GEN_SET_2_FROM_VEC3(x, y, z)
        GEN_SET_2_FROM_VEC3(r, g, b)
        GEN_SET_3_FROM_VEC3(x, y, z)
        GEN_SET_3_FROM_VEC3(r, g, b)
    };

    template <typename T, size_t S, template <typename, size_t> class DerivedT>
    struct vec4_base : public vec3_base<T, S, DerivedT> {
        void w(T _w)       { this->template get<3>() = _w; }
        T    w()     const { return this->template get<3>(); }

        void a(T _a)       { this->w(_a); }
        T    a()     const { return this->w(); }

        GEN_GET_2_FROM_VEC4(x, y, z, w)
        GEN_GET_2_FROM_VEC4(r, g, b, a)
        GEN_GET_3_FROM_VEC4(x, y, z, w)
        GEN_GET_3_FROM_VEC4(r, g, b, a)
        GEN_GET_4_FROM_VEC4(x, y, z, w)
        GEN_GET_4_FROM_VEC4(r, g, b, a)

        GEN_SET_2_FROM_VEC4(x, y, z, w)
        GEN_SET_2_FROM_VEC4(r, g, b, a)
        GEN_SET_3_FROM_VEC4(x, y, z, w)
        GEN_SET_3_FROM_VEC4(r, g, b, a)
        GEN_SET_4_FROM_VEC4(x, y, z, w)
        GEN_SET_4_FROM_VEC4(r, g, b, a)
    };


    /*
     * Universal vector
     */

    template <typename T, size_t S>
    struct vec : public vec_specific<T, S, vec> {};

    template <typename T>
    struct vec<T, 1> : public vec1_base<T, 1, vec> {
        void set(T x) {
            this->x(x);
        }
    };

    template <typename T>
    struct vec<T, 2> : public vec2_base<T, 2, vec> {
        void set(T x, T y) {
            this->x(x);
            this->y(y);
        }
    };

    template <typename T>
    struct vec<T, 3> : public vec3_base<T, 3, vec> {
        void set(T x, T y, T z) {
            this->x(x);
            this->y(y);
            this->z(z);
        }

        template <typename TT>
        vec<T, 3> cross(const vec<TT, 3>& _vec) {
            return vec{
                    this->y() * _vec.z() - this->z() * _vec.y(),
                    this->z() * _vec.x() - this->x() * _vec.z(),
                    this->x() * _vec.y() - this->y() * _vec.x()
            };
        }
    };

    template <typename T>
    struct vec<T, 4> : public vec4_base<T, 4, vec> {
        void set(T x, T y, T z, T w) {
            this->x(x);
            this->y(y);
            this->z(z);
            this->w(w);
        }
    };

    template <typename T, typename... Ts>
    vec(T, Ts...) -> vec<T, sizeof...(Ts) + 1>;

    template <typename T, size_t S>
    vec(std::array<T, S>) -> vec<T, S>;


    /*
     * Aliases
     */

    template <typename T>
    using vec2 = vec<T, 2>;

    template <typename T>
    using vec3 = vec<T, 3>;

    template <typename T>
    using vec4 = vec<T, 4>;

    using vec2i = vec2<int>;
    using vec2u = vec2<unsigned>;
    using vec2f = vec2<float>;
    using vec2d = vec2<double>;

    using vec3i = vec3<int>;
    using vec3u = vec3<unsigned>;
    using vec3f = vec3<float>;
    using vec3d = vec3<double>;

    using vec4i = vec4<int>;
    using vec4u = vec4<unsigned>;
    using vec4f = vec4<float>;
    using vec4d = vec4<double>;


    template <size_t N, typename T, size_t S>
    decltype(auto) get(const vec<T, S>& v) {
        return v.template get<N>();
    }

    template <size_t N, typename T, size_t S>
    decltype(auto) get(vec<T, S>& v) {
        return v.template get<N>();
    }

    template <typename T>
    inline auto from_glm_vec(T&& glm_vector) {
        static_assert(glm_vector.length() == 2 || glm_vector.length() == 3 || glm_vector.length() == 4);

        if constexpr (glm_vector.length() == 2)
            return vec{glm_vector.x, glm_vector.y};
        else if constexpr (glm_vector.length() == 3)
            return vec{glm_vector.x, glm_vector.y, glm_vector.z};
        else if constexpr (glm_vector.length() == 4)
            return vec{glm_vector.x, glm_vector.y, glm_vector.z, glm_vector.w};
    }
}

namespace std {
    template <typename T, size_t S>
    struct tuple_size<core::vec<T, S>> : std::integral_constant<size_t, S> {};

    template <size_t N, typename T, size_t S>
    struct tuple_element<N, core::vec<T, S>> : tuple_element<N, std::array<T, S>> {};
}

#undef GEN_GET_2
#undef GEN_GET_3
#undef GEN_GET_2_FROM_VEC2
#undef GEN_GET_2_FROM_VEC3
#undef GEN_GET_2_FROM_VEC4
#undef GEN_GET_3_FROM_VEC3
#undef GEN_GET_3_FROM_VEC4
#undef GEN_GET_4_FROM_VEC4

#undef GEN_SET_2
#undef GEN_SET_3
#undef GEN_SET_2_FROM_VEC2
#undef GEN_SET_2_FROM_VEC3
#undef GEN_SET_2_FROM_VEC4
#undef GEN_SET_3_FROM_VEC3
#undef GEN_SET_3_FROM_VEC4
#undef GEN_SET_4_FROM_VEC4

#pragma once

#include <core/config_manager.hpp>
#include <core/print.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/quaternion.hpp>

namespace core
{
template <typename T>
struct is_glm_matrix : std::false_type {};

template <glm::length_t C, glm::length_t R, FloatingPoint T, glm::qualifier Q>
struct is_glm_matrix<glm::mat<C, R, T, Q>> : std::true_type {};

template <typename T>
static constexpr bool is_glm_matrix_v = is_glm_matrix<T>::value;

template <typename T>
concept GlmMatrix = is_glm_matrix_v<T>;

template <typename T>
struct glm_mat_traits;

template <glm::length_t C, glm::length_t R, FloatingPoint T, glm::qualifier Q>
struct glm_mat_traits<glm::mat<C, R, T, Q>> {
    static constexpr glm::length_t  columns   = C;
    static constexpr glm::length_t  rows      = R;
    static constexpr glm::qualifier qualifier = Q;
    using float_t                             = T;
};

template <GlmMatrix T>
T config_cast(string_view str, const cast_helper& helper = {}) {
    constexpr auto cols = static_cast<size_t>(glm_mat_traits<T>::columns);
    constexpr auto rows = static_cast<size_t>(glm_mat_traits<T>::rows);
    using float_t       = typename glm_mat_traits<T>::float_t;

    auto array_mat = config_cast<array<array<float_t, rows>, cols>>(str, helper);
    decltype(glm::transpose(std::declval<T>())) result;
    memcpy(&result[0][0], &array_mat[0][0], sizeof(float_t) * cols * rows);

    return glm::transpose(result);
}

template <glm::length_t C, glm::length_t R, FloatingPoint T, glm::qualifier Q>
struct magic_printer<glm::mat<C, R, T, Q>> {
    void operator()(std::ostream& os, const glm::mat<C, R, T, Q>& m) {
        constexpr auto cols = static_cast<size_t>(C);
        constexpr auto rows = static_cast<size_t>(R);

        auto                        transposed = glm::transpose(m);
        array<array<T, rows>, cols> array_mat;

        memcpy(&array_mat[0][0], &transposed[0][0], sizeof(T) * cols * rows);

        magic_print(os, array_mat);
    }
};

template <>
struct magic_printer<glm::quat> {
    void operator()(std::ostream& os, const glm::quat& q) {
        magic_print(os, core::vec4f(q.w, q.x, q.y, q.z));
    }
};

} // namespace core

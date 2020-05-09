#pragma once

#include <gsl/gsl>
#include <chrono>
#include <string>
#include <deque>
#include <iostream>
#include <string_view>
#include <variant>
#include <any>
#include <memory>
#include <functional>
#include <flat_hash_map.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace core
{
    using gsl::byte;

    namespace chrono = std::chrono;
    using namespace std::literals;

    using std::function;
    using std::tuple;
    using std::tuple_size_v;
    using std::tuple_element_t;
    using std::tuple_size;
    using std::tuple_element;
    using std::pair;
    using std::vector;
    using std::deque;
    using std::string;
    using std::u16string;
    using std::u32string;
    using std::u8string;
    using std::string_view;
    using std::u16string_view;
    using std::u32string_view;
    using std::u8string_view;
    using std::array;
    using std::move;
    using std::forward;
    using std::optional;
    using std::nullopt;
    using std::variant;
    using std::any;
    using std::copy;

    using std::unique_ptr;
    using std::make_unique;

    using std::shared_ptr;
    using std::make_shared;

    using std::weak_ptr;

    template<
            typename K, typename V,
            typename H = std::hash<K>,
            typename E = std::equal_to<>,
            typename A = std::allocator<std::pair<K, V>>>
    using hash_map = ska::flat_hash_map<K, V, H, E, A>;

    template<
            typename T,
            typename H = std::hash<T>,
            typename E = std::equal_to<>,
            typename A = std::allocator<T>>
    using hash_set = ska::flat_hash_set<T, H, E, A>;

    using gsl::not_null;
    using gsl::span;
    using gsl::make_span;
    using gsl::zstring;
    using gsl::czstring;


// Concepts
    template<typename T, template <typename...> class TemplateT>
    struct is_specialization : std::false_type {};

    template<template<typename...> class TemplateT, typename... ArgsT>
    struct is_specialization <TemplateT<ArgsT...>, TemplateT>: std::true_type {};

    template <typename T>
    struct is_std_array : std::false_type {};

    template <typename T, typename array<T, 1>::size_type size>
    struct is_std_array<array<T, size>> : std::true_type {};

    template <typename T>
    concept SharedPtr = is_specialization<T, shared_ptr>::value;

    template <typename T, template <typename...> class TemplateT>
    concept SpecializationOf = is_specialization<T, TemplateT>::value;

    template <typename T>
    concept StdArray = is_std_array<T>::value;

    template <typename T>
    concept Integral = std::is_integral_v<T>;

    template <typename T>
    concept FloatingPoint = std::is_floating_point_v<T>;

    template <typename T>
    concept Unsigned = std::is_unsigned_v<T>;

    template <typename T>
    concept Number = Integral<T> || FloatingPoint<T>;

    template<typename T>
    concept Iterable = requires(T && v) {
        std::begin(v);
        std::end(v);
    };

    template<typename T>
    concept IndexAccessible = requires(T && v) {
        v[0];
    };

    template<typename T, typename... ArgsT>
    concept AnyOfType = (std::is_same_v<T, ArgsT> || ...);

    template<typename T>
    concept StringLike = IndexAccessible<T> &&
                         AnyOfType<std::decay_t<decltype(std::declval<T>()[0])>, char, char16_t, char32_t, char8_t>;

    template<typename T>
    concept ChainFunctor = requires{typename T::chain_functor;};

} // namespace core

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
#include <list>
#include <functional>
#include <algorithm>
#include <map>
#include <set>
#include <flat_hash_map.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <libcuckoo/cuckoohash_map.hh>
#include <readerwriterqueue/readerwriterqueue.h>

#include <boost/hana.hpp>
#include "try_opt.hpp"

namespace core
{
    using gsl::byte;

    using u8  = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using i8  = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using f32 = float;
    using f64 = double;

    namespace hana = boost::hana;
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
    using std::initializer_list;
    using std::list;

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

    template <typename K, typename V>
    using mpmc_hash_map = libcuckoo::cuckoohash_map<K, V>;

    template<
            typename K, typename V,
            typename Compare = std::less<K>,
            typename A = std::allocator<std::pair<const K, V>>>
    using map = std::map<K, V, Compare, A>;

    template<
            typename K,
            typename Compare = std::less<K>,
            typename A = std::allocator<K>>
    using set = std::set<K, Compare, A>;

    template <typename T, size_t MaxBlockSize = 512> // NOLINT
    using spsc_queue = moodycamel::ReaderWriterQueue<T, MaxBlockSize>;

    using gsl::not_null;
    using gsl::span;
    using gsl::make_span;
    using gsl::zstring;
    using gsl::czstring;

    template <typename T>
    using numlim = std::numeric_limits<T>;

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
    concept Optional = is_specialization<T, optional>::value;

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

    template <typename T>
    concept Enum = std::is_enum_v<T>;

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
    concept IsAdapter = requires{typename T::adapter;};

    template <typename T>
    concept Exception = std::is_base_of_v<std::exception, T>;

    template <typename T, typename F> requires IsAdapter<F>
    inline auto operator/(const T& v, F f) {
        return f(v);
    }

    template <typename T>
    struct constructor_accessor {
        using cref = const constructor_accessor&;
    private:
        friend T;
        constructor_accessor() = default;
    };

    enum class file_type { regular, directory, char_dev, block_dev, socket, pipe, symlink };

    struct file_stat {
        u64       size;
        file_type type;
    };

    template <typename F>
    struct opt_map {
        opt_map(const F& func): map_func(func) {}
        opt_map(F&& func): map_func(move(func)) {}

        using adapter = void;

        template <Optional T>
        auto operator()(const T& opt) const {
            if (opt)
                return optional{map_func(*opt)};
            else
                return optional<decltype(map_func(*opt))>{};
        }

        F map_func;
    };

    template <typename T, typename... Ts>
    constexpr bool contains_type(tuple<Ts...>&&) {
        return false || (std::is_same_v<T, Ts> || ...);
    }

    template <typename T>
    concept CopyablePointer = std::is_pointer_v<T> || is_specialization<T, shared_ptr>::value;

    template <typename T>
    concept Pointer = CopyablePointer<T> || is_specialization<T, unique_ptr>::value;

    template <CopyablePointer T>
    class safeptr {
    public:
        safeptr(T ptr): data(move(ptr)) {}

        template <typename F>
        auto operator()(F&& access_callback) {
            using R = std::invoke_result_t<F, T>;
            if constexpr (std::is_reference_v<std::remove_const_t<R>> && !Pointer<std::decay_t<R>>) {
                if (data)
                    return &access_callback(data);
                else
                    return std::add_pointer_t<std::remove_reference_t<R>>{nullptr};
            } else if constexpr (Pointer<std::decay_t<R>>) {
                if (data)
                    return access_callback(data);
                else
                    return std::decay_t<R>{nullptr};
            } else {
                if (data)
                    return optional{access_callback(data)};
                else
                    return optional<R>{nullptr};
            }
        }

    private:
        T data;
    };


} // namespace core

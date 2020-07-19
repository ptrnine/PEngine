#pragma once

#include <functional>
#include <optional>
#include <string>
#include <variant>

namespace core
{

template <typename F>
struct is_callable_impl {
private:
    using yes = char (&)[1]; // NOLINT
    using no  = char (&)[2]; // NOLINT

    struct fallback {
        void operator()();
    };
    struct derived : F, fallback {};

    template <typename U, U>
    struct check;

    template <typename>
    static yes test(...);

    template <typename C>
    static no test(check<void (fallback::*)(), &C::operator()>*);

public:
    static constexpr bool value = sizeof(test<derived>(0)) == sizeof(yes);
};

template <typename T>
struct is_callable_impl2 : std::false_type {};

template <typename R, typename... ArgsT>
struct is_callable_impl2<R (*)(ArgsT...)> : std::true_type {};

template <typename R, typename... ArgsT>
struct is_callable_impl2<R (&)(ArgsT...)> : std::true_type {};

template <typename R, typename... ArgsT>
struct is_callable_impl2<R(ArgsT...)> : std::true_type {};

template <typename F>
struct is_callable : std::conditional<std::is_class_v<F>, is_callable_impl<F>, is_callable_impl2<F>>::type {};

template <typename F>
static constexpr bool is_callable_v = is_callable<F>::value;

template <typename F>
concept Callable = is_callable<F>::value;

namespace dtls
{
    template <typename T = void>
    struct Void {
        using type = void;
    };

    template <typename R, typename... ArgsT>
    struct func_traits : std::true_type {
        using return_type = R;

        static constexpr size_t arity = sizeof...(ArgsT);

        template <size_t I>
        using arg_type_t = std::tuple_element_t<I, std::tuple<ArgsT...>>;
    };
} // namespace dtls

template <typename F, typename = void>
struct function_traits : std::false_type {
    using return_type = void;
};

template <typename R, typename... ArgsT>
struct function_traits<R (*)(ArgsT...)> : dtls::func_traits<R, ArgsT...> {};

template <typename R, typename... ArgsT>
struct function_traits<R (&)(ArgsT...)> : dtls::func_traits<R, ArgsT...> {};

template <typename R, typename... ArgsT>
struct function_traits<R(ArgsT...)> : dtls::func_traits<R, ArgsT...> {};

template <typename F>
struct function_traits<F, typename dtls::Void<decltype(&F::operator())>::type>
    : function_traits<decltype(&F::operator())> {};

template <typename F, typename R, typename... ArgsT>
struct function_traits<R (F::*)(ArgsT...) const> : dtls::func_traits<R, ArgsT...> {};

template <typename F, typename R, typename... ArgsT>
struct function_traits<R (F::*)(ArgsT...)> : dtls::func_traits<R, ArgsT...> {};

template <typename T>
concept ExplicitReturnType = function_traits<T>::value;

template <typename T, typename... ArgsT>
struct is_explicit_return_type_or_invocable : std::is_invocable<T, ArgsT...> {};

template <ExplicitReturnType T, typename... ArgsT>
struct is_explicit_return_type_or_invocable<T, ArgsT...> : std::true_type {};

template <typename T, typename... ArgsT>
struct return_type_of : std::invoke_result<T, ArgsT...> {};

template <ExplicitReturnType T, typename... ArgsT>
struct return_type_of<T, ArgsT...> {
    using type = typename function_traits<T>::return_type;
};

template <typename T, typename... ArgsT>
static constexpr bool is_explicit_return_type_or_invocable_v = is_explicit_return_type_or_invocable<T, ArgsT...>::value;

template <typename T, typename... ArgsT>
using return_type_of_t = typename return_type_of<T, ArgsT...>::type;

template <typename F>
concept MatchFunctionHolder = requires {
    typename F::match_function_holder_tag;
};

template <typename V>
concept MatchValueHolder = requires {
    typename V::match_value_holder_tag;
};

template <typename T>
concept MatchPairFunc = MatchFunctionHolder<typename T::second_type>;

template <typename T>
concept MatchPairValue = MatchValueHolder<typename T::second_type>;

template <typename T>
concept MatchPair = MatchPairFunc<T> || MatchPairValue<T>;

template <typename T>
concept MatchRow = MatchPairFunc<T> || MatchPairValue<T> || Callable<T>;

template <typename T>
concept MatchSyntax = requires {
    typename T::match_tag;
};

template <typename T>
struct is_variant {
    static constexpr bool value = false;
};

template <typename... Ts>
struct is_variant<std::variant<Ts...>> {
    static constexpr bool value = true;
};

template <typename T>
concept Variant = is_variant<T>::value;

enum class noopt_t : uint8_t {};
static constexpr noopt_t noopt = static_cast<noopt_t>(0);

template <typename F>
struct match_function_holder {
    using match_function_holder_tag = void;
    using func_type                 = F;

    F func;
};

template <typename V>
struct match_value_holder {
    using match_value_holder_tag = void;
    using value_type             = V;

    V value;
};

template <Callable F>
constexpr match_function_holder<F> operator-(F&& func) {
    return match_function_holder<F>{std::forward<F>(func)};
}

template <typename T, MatchFunctionHolder H, typename DT = std::decay_t<T>>
constexpr std::pair<DT, H> operator<(T&& condition, H&& match_func_hold) {
    return std::pair<DT, H>(std::forward<T>(condition), std::forward<H>(match_func_hold));
}

template <typename F>
struct condition_holder {
    F func;
};

template <typename F, typename FF = std::remove_reference_t<F>>
requires Callable<FF> constexpr condition_holder<F> operator--(F&& func, int) {
    return condition_holder<F>{std::forward<F>(func)};
}

constexpr noopt_t operator--(noopt_t n, int) {
    return n;
}

template <typename F, typename RF, typename RRF = std::remove_reference_t<RF>>
requires Callable<RRF> constexpr std::pair<F, match_function_holder<RRF>>
operator>(condition_holder<F>&& cond, RF&& rhs_func) {
    return std::pair<F, match_function_holder<RRF>>(cond.func, match_function_holder<RRF>{std::forward<RF>(rhs_func)});
}

template <typename RF, typename RRF = std::remove_reference_t<RF>>
requires Callable<RRF> constexpr std::pair<noopt_t, match_function_holder<RRF>> operator>(noopt_t n, RF&& rhs_func) {
    return std::pair<noopt_t, match_function_holder<RRF>>(n, match_function_holder<RRF>{std::forward<RF>(rhs_func)});
}

template <typename F, typename RF, typename RRF = std::remove_reference_t<std::decay_t<RF>>>
constexpr std::pair<F, match_value_holder<RRF>> operator>(condition_holder<F>&& cond, RF&& value) {
    return std::pair<F, match_value_holder<RRF>>(cond.func, match_value_holder<RRF>{std::forward<RF>(value)});
}

template <typename RF, typename RRF = std::remove_reference_t<std::decay_t<RF>>>
constexpr std::pair<noopt_t, match_value_holder<RRF>> operator>(noopt_t n, RF&& value) {
    return std::pair<noopt_t, match_value_holder<RRF>>(n, match_value_holder<RRF>{std::forward<RF>(value)});
}

template <typename T>
class ret {
public:
    constexpr ret(T return_value): v(std::move(return_value)) {}
    constexpr T operator()() {
        return std::move(v);
    }

private:
    T v;
};

template <MatchRow... Ts>
class match {
public:
    using match_tag = void;

    constexpr match(Ts&&... args): cases(std::forward<Ts>(args)...) {}

    std::tuple<Ts...> cases;
};

enum class uninvokable_t : uint8_t {};

template <typename T, typename... Ts>
static constexpr bool has_type_or_void_or_uninvokable_v = std::is_same_v<T, void> || std::is_same_v<T, uninvokable_t> ||
                                                          (std::is_same_v<T, Ts> || ...);

template <typename V, typename T>
struct append_to_variant_if_unique_and_non_void;

template <typename T, typename... Ts>
struct append_to_variant_if_unique_and_non_void<std::variant<Ts...>, T> {
    using type =
        std::conditional_t<has_type_or_void_or_uninvokable_v<T, Ts...>, std::variant<Ts...>, std::variant<Ts..., T>>;
};

template <typename V, typename T>
using append_to_variant_if_unique_and_non_void_t = typename append_to_variant_if_unique_and_non_void<V, T>::type;

template <typename T, typename... Ts>
struct car_type {
    using type = T;
};

template <typename T>
struct get_match_return_type_finalize;

template <typename... Ts>
struct get_match_return_type_finalize<std::variant<Ts...>> {
    using type = std::conditional_t<
        sizeof...(Ts) != 0,
        std::conditional_t<sizeof...(Ts) != 1, std::variant<Ts...>, typename car_type<Ts..., void>::type>,
        void>;
};

template <typename T>
using get_match_return_type_finalize_t = typename get_match_return_type_finalize<T>::type;

/*
template <typename MatchT, typename T, typename Enable = void>
struct match_invoke_case_type {
    using type = uninvokable_t;
};

template <typename MatchT, MatchFunctionHolder H>
struct match_invoke_case_type<MatchT, H, std::enable_if_t<is_explicit_return_type_or_invocable_v<typename
H::func_type>>> { using type = return_type_of_t<typename H::func_type>;
};

template <typename MatchT, MatchFunctionHolder H>
struct match_invoke_case_type<MatchT, H, std::enable_if_t<is_explicit_return_type_or_invocable_v<typename H::func_type,
MatchT>>> { using type = return_type_of_t<typename H::func_type, MatchT>;
};

template <typename MatchT, Callable F>
struct match_invoke_case_type<MatchT, F, std::enable_if_t<is_explicit_return_type_or_invocable_v<F, MatchT>>> {
    using type = return_type_of_t<F, MatchT>;
};
*/

template <typename MatchT, typename T>
struct match_invoke_case_type {
    using type = uninvokable_t;
};

template <typename MatchT, MatchFunctionHolder H>
requires ExplicitReturnType<typename H::func_type> struct match_invoke_case_type<MatchT, H> {
    using type = typename function_traits<typename H::func_type>::return_type;
};

template <typename MatchT, MatchFunctionHolder H>
requires std::is_invocable_v<typename H::func_type, MatchT> struct match_invoke_case_type<MatchT, H> {
    using type = std::invoke_result_t<typename H::func_type, MatchT>;
};

template <typename MatchT, Callable F>
requires ExplicitReturnType<F> struct match_invoke_case_type<MatchT, F> {
    using type = typename function_traits<F>::return_type;
};

template <typename MatchT, Callable F>
requires std::is_invocable_v<F, MatchT> struct match_invoke_case_type<MatchT, F> {
    using type = std::invoke_result_t<F, MatchT>;
};

template <typename MatchT, typename F>
using match_invoke_case_type_t = typename match_invoke_case_type<MatchT, F>::type;

template <typename MatchT, typename V, typename Tpl>
struct get_match_return_type_impl;

template <typename MatchT, typename V, MatchPairFunc T1>
struct get_match_return_type_impl<MatchT, V, std::tuple<T1>>
    : get_match_return_type_finalize<
          append_to_variant_if_unique_and_non_void_t<V, match_invoke_case_type_t<MatchT, typename T1::second_type>>> {};

template <typename MatchT, typename V, Callable T1>
struct get_match_return_type_impl<MatchT, V, std::tuple<T1>>
    : get_match_return_type_finalize<
          append_to_variant_if_unique_and_non_void_t<V, match_invoke_case_type_t<MatchT, T1>>> {};

template <typename MatchT, typename V, MatchPairValue T1>
struct get_match_return_type_impl<MatchT, V, std::tuple<T1>>
    : get_match_return_type_finalize<
          append_to_variant_if_unique_and_non_void_t<V, typename T1::second_type::value_type>> {};

template <typename MatchT, typename V, MatchPairFunc T1, MatchRow T2, MatchRow... Ts>
struct get_match_return_type_impl<MatchT, V, std::tuple<T1, T2, Ts...>>
    : get_match_return_type_impl<
          MatchT,
          append_to_variant_if_unique_and_non_void_t<V, match_invoke_case_type_t<MatchT, typename T1::second_type>>,
          std::tuple<T2, Ts...>> {};

template <typename MatchT, typename V, Callable T1, MatchRow T2, MatchRow... Ts>
struct get_match_return_type_impl<MatchT, V, std::tuple<T1, T2, Ts...>>
    : get_match_return_type_impl<MatchT,
                                 append_to_variant_if_unique_and_non_void_t<V, match_invoke_case_type_t<MatchT, T1>>,
                                 std::tuple<T2, Ts...>> {};

template <typename MatchT, typename V, MatchPairValue T1, MatchRow T2, MatchRow... Ts>
struct get_match_return_type_impl<MatchT, V, std::tuple<T1, T2, Ts...>>
    : get_match_return_type_impl<MatchT,
                                 append_to_variant_if_unique_and_non_void_t<V, typename T1::second_type::value_type>,
                                 std::tuple<T2, Ts...>> {};

namespace details
{
    template <typename MatchT, typename T>
    struct match_has_void_case_iter;

    template <typename MatchT, Callable T>
    struct match_has_void_case_iter<MatchT, T> {
        static constexpr bool value = std::is_same_v<void, match_invoke_case_type_t<MatchT, T>>;
    };

    template <typename MatchT, MatchPairFunc T>
    struct match_has_void_case_iter<MatchT, T> {
        static constexpr bool value = std::is_same_v<void, match_invoke_case_type_t<MatchT, typename T::second_type>>;
    };

    template <typename MatchT, MatchPairValue T>
    struct match_has_void_case_iter<MatchT, T> {
        static constexpr bool value = false;
    };

    template <typename T>
    struct match_has_noopt_iter {
        static constexpr bool value = false;
    };

    template <MatchPair T>
    struct match_has_noopt_iter<T> {
        static constexpr bool value = std::is_same_v<typename T::first_type, noopt_t>;
    };
} // namespace details

template <typename T, MatchRow... Ts>
static constexpr bool match_has_void_case_v = (details::match_has_void_case_iter<T, Ts>::value || ...);

template <MatchRow... Ts>
static constexpr bool match_has_noopt_condition_v = (details::match_has_noopt_iter<Ts>::value || ...);

namespace dtls
{
    template <typename T>
    struct one_arg_func : std::false_type {};

    template <ExplicitReturnType F>
    requires(function_traits<F>::arity == 1) struct one_arg_func<F> : std::true_type {};

    template <typename T>
    concept OneArgFunc = one_arg_func<T>::value;

    template <typename F>
    struct first_arg_type_or_void {
        using type = void;
    };

    template <OneArgFunc F>
    struct first_arg_type_or_void<F> {
        using type = typename function_traits<F>::template arg_type_t<0>;
    };

    template <typename T, MatchRow... Ts>
    static constexpr bool
        is_in_func_arg_types_v = (std::is_same_v<T, std::decay_t<typename first_arg_type_or_void<Ts>::type>> || ...);
} // namespace dtls

template <typename T, MatchRow... Ts>
struct match_variant_all_types_provided : std::false_type {};

template <typename... Vs, MatchRow... Ts>
struct match_variant_all_types_provided<std::variant<Vs...>, Ts...> {
    static constexpr bool value = (dtls::is_in_func_arg_types_v<Vs, Ts...> && ...);
};

template <typename T, MatchSyntax M>
struct match_has_optional_result_impl;

template <typename T, MatchRow... Ts>
struct match_has_optional_result_impl<T, match<Ts...>> {
    static constexpr bool value = !(match_has_noopt_condition_v<Ts...>) || match_has_void_case_v<T, Ts...>;
};

template <typename T, MatchSyntax M>
struct match_has_optional_result : match_has_optional_result_impl<T, M> {};

template <Variant T, MatchRow... Ts>
requires match_has_noopt_condition_v<Ts...> struct match_has_optional_result<T, match<Ts...>> {
    static constexpr bool value = match_has_void_case_v<T, Ts...>;
};

template <Variant T, MatchRow... Ts>
requires(!match_has_noopt_condition_v<Ts...>) struct match_has_optional_result<T, match<Ts...>> {
    static constexpr bool value = match_has_void_case_v<T, Ts...> || !match_variant_all_types_provided<T, Ts...>::value;
};

template <typename T, MatchSyntax M>
static constexpr bool match_has_optional_result_v = match_has_optional_result<T, M>::value;

template <typename T>
struct match_return_type_arg_count {
    static constexpr size_t value = 1;
};

template <typename... Ts>
struct match_return_type_arg_count<std::variant<Ts...>> {
    static constexpr size_t value = sizeof...(Ts);
};

template <typename MatchResT, MatchSyntax M, typename T>
struct match_return_type_void_helper {
    using type = std::conditional_t<match_has_optional_result_v<MatchResT, M>, std::optional<T>, T>;
};

template <typename MatchResT, MatchSyntax M>
struct match_return_type_void_helper<MatchResT, M, void> {
    using type = void;
};

template <typename MatchResT, typename MatchExprT>
struct match_return_type;

template <typename MatchResT, MatchRow... Ts>
struct match_return_type<MatchResT, match<Ts...>> {
    using type1 = typename get_match_return_type_impl<MatchResT, std::variant<>, std::tuple<Ts...>>::type;
    using type  = typename match_return_type_void_helper<MatchResT, match<Ts...>, type1>::type;
};

template <typename MatchResT, MatchSyntax M>
using match_return_type_t = typename match_return_type<MatchResT, M>::type;

template <typename ResT, typename T, MatchFunctionHolder H>
constexpr ResT match_foreach_call_case(const T& value, H& func_holder) {
    using case_res_type = match_invoke_case_type_t<T, H>;

    constexpr auto no_arg_invocable  = std::is_invocable_v<typename H::func_type>;
    constexpr auto one_arg_invocable = std::is_invocable_v<typename H::func_type, T>;

    static_assert(no_arg_invocable || one_arg_invocable, "Match syntax error: case function not invocable");

    if constexpr (no_arg_invocable) {
        if constexpr (std::is_same_v<void, case_res_type>) {
            func_holder.func();
            if constexpr (std::is_same_v<void, ResT>)
                return;
            else
                return std::nullopt;
        }
        else {
            return func_holder.func();
        }
    }
    else if constexpr (one_arg_invocable) {
        if constexpr (std::is_same_v<void, case_res_type>) {
            func_holder.func(value);
            if constexpr (std::is_same_v<void, ResT>)
                return;
            else
                return std::nullopt;
        }
        else {
            if constexpr (std::is_same_v<void, ResT>)
                func_holder.func(value);
            else
                return func_holder.func(value);
        }
    }
}

template <typename ResT, typename T, MatchValueHolder H>
constexpr ResT match_foreach_call_case(const T&, H& value_holder) {
    return ResT{std::move(value_holder.value)};
}

template <typename T1, typename T2>
concept MatchComparable = requires(T1 v1, T2 v2) {
    { v1 == v2 }
    ->std::same_as<bool>;
};

template <typename T1, typename T2>
concept MatchNotEqualAllow = requires(T1 v1, T2 v2) {
    { v1 != v2 }
    ->std::same_as<bool>;
};

template <typename T1, typename T2>
concept MatchGreaterAllow = requires(T1 v1, T2 v2) {
    { v1 > v2 }
    ->std::same_as<bool>;
};

template <typename T1, typename T2>
concept MatchLessAllow = requires(T1 v1, T2 v2) {
    { v1 < v2 }
    ->std::same_as<bool>;
};

template <typename T1, typename T2>
concept MatchGreateOrEqualAllow = requires(T1 v1, T2 v2) {
    { v1 >= v2 }
    ->std::same_as<bool>;
};

template <typename T1, typename T2>
concept MatchLessOrEqualAllow = requires(T1 v1, T2 v2) {
    { v1 <= v2 }
    ->std::same_as<bool>;
};

/*================================ Placeholder ===================================*/

enum class match_placeholder_t : uint8_t {};
static constexpr match_placeholder_t _it_ = static_cast<match_placeholder_t>(0);

#define MACRO_MATCH_OP_GEN(OP, CONSTRAINT)                                                                             \
    template <typename T>                                                                                              \
    constexpr auto operator OP(match_placeholder_t, T&& rhs) {                                                         \
        return [v = std::forward<T>(rhs)](const auto& lhs) {                                                           \
            if constexpr (Variant<std::decay_t<decltype(lhs)>>) {                                                      \
                return std::visit(                                                                                     \
                    [v](auto&& arg) -> bool {                                                                          \
                        if constexpr (CONSTRAINT<std::decay_t<T>, std::decay_t<decltype(arg)>>)                        \
                            return arg OP v;                                                                           \
                        else                                                                                           \
                            return false;                                                                              \
                    },                                                                                                 \
                    lhs);                                                                                              \
            }                                                                                                          \
            else {                                                                                                     \
                return lhs OP v;                                                                                       \
            }                                                                                                          \
        };                                                                                                             \
    }

MACRO_MATCH_OP_GEN(==, MatchComparable)
MACRO_MATCH_OP_GEN(!=, MatchNotEqualAllow)
MACRO_MATCH_OP_GEN(>, MatchGreaterAllow)
MACRO_MATCH_OP_GEN(<, MatchLessAllow)
MACRO_MATCH_OP_GEN(>=, MatchGreateOrEqualAllow)
MACRO_MATCH_OP_GEN(<=, MatchLessOrEqualAllow)

#undef MACRO_MATCH_OP_GEN

template <typename ResT, size_t I = 0, typename T, MatchRow... Ts>
constexpr ResT match_foreach(const T& value, match<Ts...>& match_syntax) {
    if constexpr (I == sizeof...(Ts)) {
        return ResT();
    }
    else if constexpr (Callable<std::tuple_element_t<I, std::tuple<Ts...>>>) {
        auto& func      = std::get<I>(match_syntax.cases);
        using func_type = std::tuple_element_t<I, std::tuple<Ts...>>;

        if constexpr (std::is_invocable_v<func_type, T>) {
            if constexpr (std::is_same_v<void, std::invoke_result_t<func_type, T>>) {
                func(value);
                return;
            }
            else
                return func(value);
        }
        else if constexpr (Variant<T>) {
            if constexpr (function_traits<func_type>::value) {
                if constexpr (std::is_same_v<void, typename function_traits<func_type>::return_type>) {
                    if (std::visit(
                        [&](auto&& v) -> bool {
                            if constexpr (function_traits<func_type>::arity == 1) {
                                if constexpr (std::is_same_v<std::decay_t<decltype(v)>,
                                                             std::decay_t<typename function_traits<
                                                                 func_type>::template arg_type_t<0>>>) {
                                    func(v);
                                    return true;
                                }
                            }
                            return false;
                        },
                        value))
                    return;
                }
                else {
                    if (auto v = std::visit(
                        [&](auto&& v) -> std::optional<typename function_traits<func_type>::return_type> {
                            if constexpr (function_traits<func_type>::arity == 1)
                                if constexpr (std::is_same_v<std::decay_t<decltype(v)>,
                                                             std::decay_t<typename function_traits<
                                                                 func_type>::template arg_type_t<0>>>)
                                    return func(v);
                            return std::nullopt;
                        },
                        value))
                    return *v;
                }
            }
            else {
                if (std::visit(
                        [&](auto&& v) {
                            if constexpr (std::is_invocable_v<func_type, std::decay_t<decltype(v)>>) {
                                func(v);
                                return true;
                            }
                            return false;
                        },
                        value))
                    return;
            }
        }

        return match_foreach<ResT, I + 1>(value, match_syntax);
    }
    else if constexpr (MatchPair<std::tuple_element_t<I, std::tuple<Ts...>>>) {
        auto& cs        = std::get<I>(match_syntax.cases);
        using case_type = std::tuple_element_t<I, std::tuple<Ts...>>;

        if constexpr (is_callable_v<typename case_type::first_type>) {
            if constexpr (std::is_invocable_v<typename case_type::first_type, T>) {
                if (cs.first(value))
                    return match_foreach_call_case<ResT>(value, cs.second);
            }
            else if constexpr (Variant<T>) {
                if (std::visit(
                        [&](auto&& v) {
                            if constexpr (std::is_invocable_v<typename case_type::first_type,
                                                              std::decay_t<decltype(v)>>)
                                return cs.first(v);
                            else
                                return false;
                        },
                        value))
                    return match_foreach_call_case<ResT>(value, cs.second);
            }
        }
        else if constexpr (std::is_same_v<noopt_t, typename case_type::first_type>) {
            return match_foreach_call_case<ResT>(value, cs.second);
        }
        else if constexpr (MatchComparable<T, typename case_type::first_type>) {
            if (value == cs.first)
                return match_foreach_call_case<ResT>(value, cs.second);
        }
        else if constexpr (MatchComparable<typename case_type::first_type, T>) {
            if (cs.first == value)
                return match_foreach_call_case<ResT>(value, cs.second);
        }

        return match_foreach<ResT, I + 1>(value, match_syntax);
    }
}

template <typename T, MatchRow... Ts>
constexpr auto operator/(const T& value, match<Ts...> match_syntax) -> match_return_type_t<T, match<Ts...>> {
    using ResT = match_return_type_t<T, match<Ts...>>;
    return match_foreach<ResT>(value, match_syntax);
}

template <typename T, typename F>
requires Variant<std::decay_t<T>>&& Callable<std::decay_t<F>> auto operator/(T&& value, F&& func) {
    return std::visit(std::forward<F>(func), std::forward<T>(value));
}

} // namespace core

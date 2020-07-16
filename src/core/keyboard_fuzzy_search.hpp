#pragma once

#include "container_extensions.hpp"
#include "types.hpp"
#include "vec.hpp"

namespace core
{
namespace details
{
    constexpr array keyboard_array = {array{'1', '2', '3', '4', '5', '6', '7', '8', '9', '0'},
                                      array{'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P'},
                                      array{'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';'},
                                      array{'Z', 'C', 'X', 'V', 'B', 'N', 'M', ',', '.', '/'}};

    constexpr size_t keyboard_rows    = keyboard_array.size();
    constexpr size_t keyboard_columns = keyboard_array.front().size();

    template <size_t... Idxs>
    constexpr auto iota_vec(std::index_sequence<Idxs...>) {
        return array{Idxs...};
    }

    template <size_t S>
    constexpr auto iota_vec() {
        return iota_vec(std::make_index_sequence<S>());
    }

    template <size_t... Idxs>
    constexpr vec<size_t, 2> charcode_keyboard_pos_f(size_t charcode, std::index_sequence<Idxs...>) {
        return (
            ((charcode >= 'a' && charcode <= 'z'
                  ? static_cast<char>(charcode + 'A' - 'a') ==
                        std::get<Idxs % keyboard_columns>(std::get<Idxs / keyboard_columns>(keyboard_array))
                  : static_cast<char>(charcode) == std::get<Idxs % keyboard_columns>(std::get<Idxs / keyboard_columns>(keyboard_array)))
                 ? vec{Idxs / keyboard_columns, Idxs % keyboard_columns}
                 : vec<size_t, 2>{0, 0}) +
            ...);
    }

    constexpr vec<size_t, 2> charcode_keyboard_pos_f(size_t charcode) {
        return charcode_keyboard_pos_f(charcode, std::make_index_sequence<keyboard_rows * keyboard_columns>());
    }

    constexpr auto charcode_to_keyboard_pos =
        array_map(iota_vec<256>(), [](auto c) { return charcode_keyboard_pos_f(c); });

    inline ssize_t abs(ssize_t n) {
        return n < 0 ? -n : n;
    }

} // namespace details


/**
 * @brief Returns keyboard distance between two symbols
 *
 * @param target - symbol
 * @param actual - another symbol
 *
 * @return height + width distance between two keyboard symbols
 */
size_t kf_char_distance(char target, char actual) {
    auto [ti, tj] = details::charcode_to_keyboard_pos[static_cast<u8>(target)]; // NOLINT
    auto [ai, aj] = details::charcode_to_keyboard_pos[static_cast<u8>(actual)]; // NOLINT

    return static_cast<size_t>(details::abs(static_cast<ssize_t>(ti) - static_cast<ssize_t>(ai)) +
                               details::abs(static_cast<ssize_t>(tj) - static_cast<ssize_t>(aj)));
}

template <typename IterT1, typename IterT2>
size_t kf_iter_distance(IterT1 t, IterT2 a, size_t count) {
    size_t cost = 0;

    for (size_t i = 0; i < count; ++i) cost += kf_char_distance(*(t + i), *(a + i));

    return cost;
}

/**
 * @brief Distance between target and actual strings
 *
 * @tparam StrT1 - type of target string
 * @tparam StrT2 - type of actual string
 * @param target - target string, is what the 'actual' string wants to be
 * @param actual - actual string
 *
 * @return distance between strings
 */
template <typename StrT1, typename StrT2>
size_t kf_distance(const StrT1& target, const StrT2& actual) {
    constexpr size_t diff_symbols_cost = 6;
    size_t cost = 0;

    size_t min = target.size();
    size_t max = actual.size();

    if (min > max)
        std::swap(min, max);

    cost += (max - min) * diff_symbols_cost;

    for (auto [tc, ac] : zip_view(target, actual)) cost += kf_char_distance(tc, ac);

    for (size_t ti = 0, ai = 0; ti < target.size() && ai < actual.size(); ++ti, ++ai) {
        if (ai != actual.size() - 1 && kf_char_distance(target[ti], actual[ai]) > 1) {
            auto cur_cost   = kf_iter_distance(target.begin() + ti, actual.begin() + ai + 0, min - ti);
            auto shift_cost = kf_iter_distance(target.begin() + ti, actual.begin() + ai + 1, min - ti);

            if (cur_cost > shift_cost) {
                ++ai;
                cost -= diff_symbols_cost;
            }
        }
    }

    return cost;
}

/**
 * @brief Dumb fuzzy search with keyboard distance
 *
 * @tparam ResultsCount - count of resulting values
 * @tparam T - type of container with strings
 * @param container - container with strings
 * @param check_value - fuzzy string
 *
 * @return array with sorted results
 */
template <size_t ResultsCount = 1, Iterable T>
array<typename T::const_iterator, ResultsCount> kf_search(const T& container, const string& check_value) {
    static_assert(ResultsCount != 0);

    vector<pair<size_t, size_t>> sorted_proxy(container.size());

    auto vi = value_index_view(container);
    for (auto& [value_index, dst] : zip_view(vi, sorted_proxy)) {
        auto& [value, index] = value_index;
        dst = {kf_distance(value, check_value), index};
    }

    auto less_func = [](auto& lhs, auto& rhs) {
        return lhs.first < rhs.first;
    };

    std::partial_sort(sorted_proxy.begin(), sorted_proxy.begin() + ResultsCount, sorted_proxy.end(), less_func);

    array<typename T::const_iterator, ResultsCount> result;
    std::transform(sorted_proxy.begin(), sorted_proxy.begin() + ResultsCount, result.begin(), [&](auto& v) {
        return container.begin() + v.second;
    });

    return result;
}

/**
 * @brief Sorting range of fuzzy strings
 *
 * @tparam IterT - type of the iterator
 * @param begin - begin of the range
 * @param end - end of the range
 * @param target - exact string
 */
template <typename IterT>
void kf_sort(IterT begin, IterT end, const string& target) {
    std::stable_sort(begin, end, [&](const auto& lhs, const auto& rhs) {
        return estimate_string(target, lhs) < estimate_string(target, rhs);
    });
}

} // namespace core

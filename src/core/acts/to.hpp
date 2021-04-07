#pragma once

#include "../range.hpp"
#include "../views/sub.hpp"

namespace core::acts
{
template <typename Container>
struct to {
    template <typename I>
    Container operator()(I begin, I end) const {
        using T  = std::decay_t<decltype(*begin)>;
        using CT = std::decay_t<decltype(*core::begin(std::declval<Container>()))>;
        if constexpr (Range<CT> && !std::is_same_v<CT, T>) {
            auto subfix = range{begin, end} / views::sub(to<CT>());
            return Container(core::begin(subfix), core::end(subfix));
        }
        else
            return Container(begin, end);
    }

    friend Container operator/(Range auto&& rng, const to& to) {
        using std::begin, std::end;
        return to(begin(rng), end(rng));
    }
};
struct to_vector {
    template <typename I>
    auto operator()(I begin, I end) const {
        return vector(begin, end);
    }

    friend auto operator/(Range auto&& rng, const to_vector& to) {
        using std::begin, std::end;
        return to(begin(rng), end(rng));
    }
};
} // namespace core::acts

#include "core/vec.hpp"
#include <core/types.hpp>
#include <core/container_extensions.hpp>
#include <random>
#include <catch2/catch.hpp>

using namespace core;

TEST_CASE("For loop variations") {
    SECTION("Index loop") {
        vector<size_t> v1(100);
        vector<size_t> v2(100);

        for (size_t i = 0; i < v1.size(); ++i)
            v1[i] = i;

        for (auto i : index_view(v2))
            v2[i] = i;

        REQUIRE(v1 == v2);
    }

    SECTION("Value loop with index") {
        vector<size_t> v1(100);
        vector<size_t> v2(100);

        for (size_t i = 0; auto& v : v1)
            v = i++;

        for (auto& [v, i] : value_index_view(v2))
            v = i;

        for (auto [v, i] : value_index_view(v2))
            v += i; // Not affect to v2

        REQUIRE(v1 == v2);
    }

    SECTION("Zip view") {
        vector<size_t> a(100);
        vector<size_t> b(100);

        std::generate(b.begin(), b.end(), std::bind(std::uniform_int_distribution<size_t>(0, 1000), std::mt19937()));

        for (auto& [ai, bi] : zip_view(a, b))
            ai = bi;

        for (auto [ai, bi] : zip_view(a, b))
            ai = bi; // Not affect to ai

        REQUIRE(a == b);
    }

    SECTION("Adapters") {
        vector a{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}; // NOLINT

        REQUIRE(a / count_if(xlambda(a, a > 4)) == 6);
        REQUIRE(a / count_if(xlambda(a, a > 10)) == 0);

        REQUIRE(a / any_of(xlambda(a, a == 9)) == true);
        REQUIRE(a / any_of(xlambda(a, a == -11)) == false);

        REQUIRE(a / all_of(xlambda(a, a > 0 && a <= 10)) == true);
        REQUIRE(a / all_of(xlambda(a, a > 0 && a < 10)) == false);

        REQUIRE(a / find_if(xlambda(a, a == 3)) != a.end());
        REQUIRE(a / find_if(xlambda(a, a == -3)) == a.end());

        REQUIRE(a / transform<vector<string>>(xlambda(a, std::to_string(a))) == vector<string>{
            "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"
        });

        REQUIRE(a / is_sorted() == true);
        a.front() = 100; // NOLINT
        REQUIRE(a / is_sorted() == false);
        a.front() = 1;

        REQUIRE(a / adjacent_difference() == vector{1, 1, 1, 1, 1, 1, 1, 1, 1, 1});

        REQUIRE(a / accumulate(string(), xlambda2(e, r, e + std::to_string(r))) == "12345678910");
        REQUIRE(a / reduce(0, std::plus<>()) == 55);

        REQUIRE(vector{"one"s, "two"s, "three"s} / fold("|") == "one|two|three");
        REQUIRE(vector{vector{1, 2}, vector{3, 4}, vector{5, 6, 7}} / fold(8) == vector{1, 2, 8, 3, 4, 8, 5, 6, 7});
        REQUIRE(vector{vector{1, 2}, vector{3, 4}, vector{5, 6, 7}} / fold(array{8}) == vector{1, 2, 8, 3, 4, 8, 5, 6, 7});

        REQUIRE("228" / to_number<int>() == 228);
        REQUIRE(essentially_equal("228.228" / to_number<float>(), 228.228f, 0.0001f)); // NOLINT

        REQUIRE("    kwek        "s / remove_trailing_whitespaces() == "kwek");
        REQUIRE("a b c d" / split_view(' ') == vector<string_view>{"a", "b", "c", "d"});
        REQUIRE("a |b c d |" / split_view({' ', '|'}) == vector<string_view>{"a", "b", "c", "d"});
        REQUIRE("a |b c d |" / split_view({' ', '|'}, true) == vector<string_view>{"a", "", "b", "c", "d", ""});
        REQUIRE("a |b c d |" / split(vector{' ', '|'}) == vector<string>{"a", "b", "c", "d"});
        REQUIRE("a |b c d |" / split(vector{' ', '|'}, true) == vector<string>{"a", "", "b", "c", "d", ""});
    }

    SECTION("Other shit") {
        REQUIRE("one"s / "two" / "three" == "one/two/three");
        REQUIRE(case_insensitive_match("KeK", "kek") == true);
        REQUIRE(case_insensitive_match("KVK", "kek") == false);
    }


}

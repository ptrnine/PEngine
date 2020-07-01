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

        REQUIRE(v1 == v2);
    }

    SECTION("Zip view") {
        vector<size_t> a(100);
        vector<size_t> b(100);

        std::generate(b.begin(), b.end(), std::bind(std::uniform_int_distribution<size_t>(0, 1000), std::mt19937()));

        for (auto& [ai, bi] : zip_view(a, b))
            ai = bi;

        REQUIRE(a == b);
    }
}

#include <catch2/catch.hpp>
#include <core/base64.hpp>
#include <core/container_extensions.hpp>
#include <random>

using namespace core;

vector<byte> bytevec(string_view str) {
    return str / transform<vector<byte>>(xlambda(c, static_cast<byte>(c)));
}

TEST_CASE("base64") {
    auto v = base64_encode("");
    REQUIRE(v.empty());
    REQUIRE(base64_decode(v).empty());

    auto a1 = base64_encode("A");
    REQUIRE(base64_decode(a1) == bytevec("A"));

    auto a2 = base64_encode("AB");
    REQUIRE(base64_decode(a2) == bytevec("AB"));

    auto a3 = base64_encode("ABC");
    REQUIRE(base64_decode(a3) == bytevec("ABC"));

    auto a4 = base64_encode("ABCD");
    REQUIRE(base64_decode(a4) == bytevec("ABCD"));

    auto a5 = base64_encode("Lorem ipsum dolor sit amet");
    REQUIRE(base64_decode(a5) == bytevec("Lorem ipsum dolor sit amet"));

    auto a6 = base64_encode("Lorem ipsum dolor sit amet.");
    REQUIRE(base64_decode(a6) == bytevec("Lorem ipsum dolor sit amet."));

    auto a7 = base64_encode("Lorem ipsum dolor sit ame");
    REQUIRE(base64_decode(a7) == bytevec("Lorem ipsum dolor sit ame"));

    auto genchar = std::bind(std::uniform_int_distribution<char>('A', 'z'), std::mt19937()); // NOLINT
    auto gensz   = std::bind(std::uniform_int_distribution<size_t>(1, 1000), std::mt19937()); // NOLINT
    for (int i = 0; i < 1000; ++i) {
        std::string str(gensz(), ' ');
        std::generate(str.begin(), str.end(), genchar);
        auto en = base64_encode(str);
        REQUIRE(base64_decode(en) == bytevec(str));
    }
}


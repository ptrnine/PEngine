#include <catch2/catch.hpp>
#include <core/container_extensions.hpp>
#include <core/match_syntax.hpp>

using namespace core;

TEST_CASE("match syntax") {
    SECTION("Constexpr test") {
        constexpr int v = 5;
        constexpr string_view a1 = v / match {
            (_it_ == 1) --> "one",
            (_it_ == 2) --> [] { return "two"; },
            3            <- [] { return "three"; },
            5            <- ret("five"),
            noopt        <- ret("noopt")
        };
        REQUIRE(a1 == "five");
    }

    SECTION("No return") {
        auto m = match {
            (_it_ == 1) --> []{},
            2            <- []{}
        };
        static_assert(std::is_same_v<decltype(1/m), void>);

        int a = 0;
        std::string str = "str";
        str / match {
            "kek"            <- [&]{ a += 10; },
            (_it_ == "mda") --> [&]{ a += 20; },
            (_it_ == "str") --> [&]{ a += 30; }
        };
        REQUIRE(a == 30);

        int a1 = 0;
        "KEK" / match {
            "kek"s <- [&]{ a1 += 10; },
            noopt  <- [&]{ a1 += 20; }
        };
        REQUIRE(a1 == 20);
    }

    SECTION("One return") {
        auto m1 = match {
            "Hello" <- xlambda(it, it + ", world!"),
            (_it_ == "What's up") <- xlambda(it, it + " man")
        };
        REQUIRE("Hello"s / m1     == "Hello, world!");
        REQUIRE("What's up"s / m1 == "What's up man");

        auto a1 = 5 / match {
            2 <- ret("hello"s),
            5 <- ret("bye"s)
        };
        static_assert(std::is_same_v<decltype(a1), optional<string>>);
        REQUIRE(a1.value() == "bye");
    }

    SECTION("Variant matching and return") {
        variant<string, int> v1 = "string";

        auto a1 = v1 / match {
            (_it_ == 1)        --> 1,
            (_it_ == "string") --> 2,
            noopt              --> 3
        };
        REQUIRE(a1 == 2);

        string a2;
        v1 / match{
            [&](const string& str) { a2 = str + "kek"; },
            [&](const int& a)      { a2 = std::to_string(a); }
        };
        REQUIRE(a2 == "stringkek");

        v1 = 1;
        auto a3 = v1 / match {
            (_it_ == "kek") --> true,
            (_it_ > 1)      --> 1,
            (_it_ < 2)      --> "hello"
        };
        static_assert(std::is_same_v<decltype(a3), optional<variant<bool, int, const char*>>>);
        REQUIRE(std::get<const char*>(a3.value()) == "hello"s);

        variant<string, int, float> v2 = 222.2f;
        auto a4 = v2 / match {
            [](const string&) { return 1; },
            [](int)           { return 2; }
        };
        static_assert(std::is_same_v<decltype(a4), optional<int>>);
        REQUIRE(a4.has_value() == false);

        auto a5 = v2 / match {
            [](const string&) { return 1; },
            noopt --> 2
        };
        static_assert(std::is_same_v<decltype(a5), int>);
        REQUIRE(a5 == 2);

        auto a6 = v2 / match {
            [](const string&) { return 1; },
            [](int)           { return 2; },
            [](float)         { return 3; }
        };
        static_assert(std::is_same_v<decltype(a6), int>);
        REQUIRE(a6 == 3);
    }
}

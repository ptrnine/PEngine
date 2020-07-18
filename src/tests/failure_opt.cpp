#include <core/types.hpp>
#include <core/failure_opt.hpp>
#include <core/helper_macros.hpp>
#include <catch2/catch.hpp>

TEST_CASE("failure_opt") {
    using core::failure_opt;
    using core::string;

    failure_opt<int> a1 = 2;
    failure_opt<int> a2;

    REQUIRE(a1);
    REQUIRE(!a2);

    auto a3 = a1.map(xlambda(x, std::to_string(x)));
    auto a4 = a2.map(xlambda(x, std::to_string(x)));

    static_assert(std::is_same_v<failure_opt<string>, decltype(a3)>);
    REQUIRE(a3);
    REQUIRE(!a4);

    a3 = std::runtime_error("kek");

    REQUIRE(!a3);
    try {
        *a3 += "ewr";
    } catch (std::exception& e) {
        REQUIRE(string("kek") == e.what());
    }
    a3.emplace("KEK");

    REQUIRE(a3.value() == "KEK");
}


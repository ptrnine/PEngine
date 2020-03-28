#include <catch2/catch.hpp>
#include <core/print.hpp>
#include <core/vec.hpp>

TEST_CASE("print") {
    using namespace core;

    SECTION("format") {
        REQUIRE("1" == format("{}", 1));
        REQUIRE("str" == format("{}", string("str")));
        REQUIRE("true" == format("{}", true));
        REQUIRE("1.5" == format("{}", 1.5));
        REQUIRE("{ 1, false }" == format("{}", pair{1, false}));
        REQUIRE("{ 1, 2, 3, 4, 5 }" == format("{}", vector{1, 2, 3, 4, 5}));
        REQUIRE("{ 1, 2, 3, 4 }" == format("{}", array{1, 2, 3, 4}));
        REQUIRE("{ false, 1.5, 10 }" == format("{}", tuple{false, 1.5, 10}));
        REQUIRE("{ 10, 10, 20 } { 10, 20 } { 1, 2, 3, 4 }" == format("{} {} {}", vec{10, 10, 20}, vec{10, 20}, vec{1, 2, 3, 4}));
        REQUIRE("nullopt" == format("{}", optional<vector<tuple<int, float, pair<int, int>>>>()));
        REQUIRE("1" == format("{}", optional{1}));
    }

    SECTION("member function") {
        class printable_shit {
        public:
            printable_shit(int ia, float ib): a(ia), b(ib) {}

            void print(std::ostream& os) const {
                os << "[ " << a << ", " << b << " ]";
            }

        private:
            int a;
            float b;
        };

        REQUIRE("[ 1, 1 ]" == format("{}", printable_shit(1, 1.0f)));
    }
}
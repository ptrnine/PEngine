#include <catch2/catch.hpp>
#include <core/vec.hpp>

TEST_CASE("vec") {
    using namespace core;

    SECTION("from glm") {
        glm::vec2 a = { 1, 2 };
        glm::vec3 b = { 1, 2, 3 };
        glm::vec4 c = { 1, 2, 3, 4 };

        REQUIRE(vec{1.f, 2.f}.essentially_equal(from_glm(a), 0.001f));
        REQUIRE(vec{1.f, 2.f, 3.f}.essentially_equal(from_glm(b), 0.001f));
        REQUIRE(vec{1.f, 2.f, 3.f, 4.f}.essentially_equal(from_glm(c), 0.001f));
    }
}

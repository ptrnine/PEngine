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

    SECTION("basic operations") {
        auto v1 = vec{1.f, 2.f};
        auto v2 = vec{1.f, 2.f, 3.f};
        auto v3 = vec{1.f, 2.f, 3.f, 4.f};

        auto v4 = vec{1.0, 2.0};
        auto v5 = vec{1.0, 2.0, 3.0};
        auto v6 = vec{1.0, 2.0, 3.0, 4.0};

        auto v7 = vec{1, 2};
        auto v8 = vec{1, 2, 3};
        auto v9 = vec{1, 2, 3, 4};

        static_assert(std::is_same_v<vec2f, decltype(v1)>);
        static_assert(std::is_same_v<vec3f, decltype(v2)>);
        static_assert(std::is_same_v<vec4f, decltype(v3)>);

        static_assert(std::is_same_v<vec2d, decltype(v4)>);
        static_assert(std::is_same_v<vec3d, decltype(v5)>);
        static_assert(std::is_same_v<vec4d, decltype(v6)>);

        static_assert(std::is_same_v<vec2i, decltype(v7)>);
        static_assert(std::is_same_v<vec3i, decltype(v8)>);
        static_assert(std::is_same_v<vec4i, decltype(v9)>);

        #define GEN_OP_TEST(NAME, OP) \
        auto NAME##1 = v1 OP v4; \
        auto NAME##2 = v1 OP v7; \
        auto NAME##3 = v4 OP v7; \
        auto NAME##4 = v3 OP v6; \
        auto NAME##5 = v3 OP v9; \
        auto NAME##6 = v6 OP v9; \
        static_assert(std::is_same_v<vec2d, decltype(NAME##1)>); \
        static_assert(std::is_same_v<vec2f, decltype(NAME##2)>); \
        static_assert(std::is_same_v<vec2d, decltype(NAME##3)>); \
        static_assert(std::is_same_v<vec4d, decltype(NAME##4)>); \
        static_assert(std::is_same_v<vec4f, decltype(NAME##5)>); \
        static_assert(std::is_same_v<vec4d, decltype(NAME##6)>)


        #define CHECK(v, ...) \
            REQUIRE(v.essentially_equal(__VA_ARGS__, 0.0001)); \
            REQUIRE(v.approx_equal(__VA_ARGS__, 0.0001))

        GEN_OP_TEST(a, +);
        GEN_OP_TEST(b, -);
        GEN_OP_TEST(c, *);
        GEN_OP_TEST(d, /);

        CHECK(a1, vec{2.0, 4.0});
        CHECK(a2, vec{2.f, 4.f});
        CHECK(a3, vec{2.0, 4.0});
        CHECK(a4, vec{2.0, 4.0, 6.0, 8.0});
        CHECK(a5, vec{2.f, 4.f, 6.f, 8.f});
        CHECK(a6, vec{2.0, 4.0, 6.0, 8.0});

        CHECK(b1, vec{0.0, 0.0});
        CHECK(b2, vec{0.f, 0.f});
        CHECK(b3, vec{0.0, 0.0});
        CHECK(b4, vec{0.0, 0.0, 0.0, 0.0});
        CHECK(b5, vec{0.f, 0.f, 0.f, 0.f});
        CHECK(b6, vec{0.0, 0.0, 0.0, 0.0});

        CHECK(c1, vec{1.0, 4.0});
        CHECK(c2, vec{1.f, 4.f});
        CHECK(c3, vec{1.0, 4.0});
        CHECK(c4, vec{1.0, 4.0, 9.0, 16.0});
        CHECK(c5, vec{1.f, 4.f, 9.f, 16.f});
        CHECK(c6, vec{1.0, 4.0, 9.0, 16.0});

        CHECK(d1, vec{1.0, 1.0});
        CHECK(d2, vec{1.f, 1.f});
        CHECK(d3, vec{1.0, 1.0});
        CHECK(d4, vec{1.0, 1.0, 1.0, 1.0});
        CHECK(d5, vec{1.f, 1.f, 1.f, 1.f});
        CHECK(d6, vec{1.0, 1.0, 1.0, 1.0});

        #define GEN_OP2_TEST(NAME, OP) \
        NAME##1 OP v4; \
        NAME##4 OP v6;

        GEN_OP2_TEST(a, +=);
        GEN_OP2_TEST(b, -=);
        GEN_OP2_TEST(c, *=);
        GEN_OP2_TEST(d, /=);

        CHECK(a1, vec{3.0, 6.0});
        CHECK(a4, vec{3.0, 6.0, 9.0, 12.0});
        CHECK(b1, vec{-1.0, -2.0});
        CHECK(b4, vec{-1.0, -2.0, -3.0, -4.0});
        CHECK(c1, vec{1.0, 8.0});
        CHECK(c4, vec{1.0, 8.0, 27.0, 64.0});
        CHECK(d1, vec{1.0, 0.5});
        CHECK(d4, vec{1.0, 0.5, 0.33333, 0.25});
    }

    SECTION("structured bindings") {
        auto v1 = vec{1, 2, 3};

        auto [x, y, z] = v1;
        REQUIRE(x == 1);
        REQUIRE(y == 2);
        REQUIRE(z == 3);

        x += 5; y += 5; z += 5;
        REQUIRE(v1 == vec{1, 2, 3});

        auto& [r, g, b] = v1;
        r = 255; g = 254; b = 253;
        REQUIRE(v1 == vec{255, 254, 253});
    }

    SECTION("Math operation") {
        REQUIRE(vec{1.0, 2.0, 3.0}.cross(vec{3.0, 4.0, -7.0}).essentially_equal(vec{-26.0, 16.0, -2.0}, 0.00001));
        REQUIRE(vec{13.0, -212.0, 33.0}.cross(vec{-0.42, -24.0, 42.0})
                .essentially_equal(vec{-8112.0, -559.86, -401.04}, 0.00001));

        REQUIRE(approx_equal(vec{1.0, 2.0, 3.0}.dot(vec{4.0, -5.0, 6.0}), 12.0, 0.00001));
        REQUIRE(approx_equal(vec{1.0, 2.0, 3.0, 4.0}.dot(vec{4.0, -5.0, 6.0, 8.0}), 44.0, 0.00001));
        REQUIRE(approx_equal(vec{1.0, 2.0}.dot(vec{4.0, -5.0}), -6.0, 0.00001));

        REQUIRE(approx_equal(vec{2.2, 3.3, 4.4}.magnitude_2(), 35.09, 0.00001));
        REQUIRE(approx_equal(vec{2.2, 3.3, 4.4}.magnitude(), 5.92368, 0.00001));
        REQUIRE(approx_equal(vec{2.2, 3.3, 4.4}.fast_magnitude(), 5.92368, 0.00001));
        REQUIRE(approx_equal(vec{2.2, 3.3, 4.4}.fast_inverse_magnitude(), 0.16881398, 0.00001));
        REQUIRE(vec{1.1, 2.2, -3.3}.normalize().approx_equal(vec{0.26726, 0.5345, -0.8018}, 0.0001));
        REQUIRE(vec{1.1, 2.2, -3.3}.fast_normalize().approx_equal(vec{0.26726, 0.5345, -0.8018}, 0.0001));
    }

    SECTION("getters and setters") {
        /* Generated by script */
        REQUIRE(vec{0, 1}.x() == 0);
        REQUIRE([](){ auto v = vec2i::filled_with(-1); v.x(0); return v; }() == vec{0, -1});
        REQUIRE(vec{0, 1}.y() == 1);
        REQUIRE([](){ auto v = vec2i::filled_with(-1); v.y(1); return v; }() == vec{-1, 1});
        REQUIRE(vec{0, 1, 2}.x() == 0);
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.x(0); return v; }() == vec{0, -1, -1});
        REQUIRE(vec{0, 1, 2}.y() == 1);
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.y(1); return v; }() == vec{-1, 1, -1});
        REQUIRE(vec{0, 1, 2}.z() == 2);
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.z(2); return v; }() == vec{-1, -1, 2});
        REQUIRE(vec{0, 1, 2, 3}.x() == 0);
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.x(0); return v; }() == vec{0, -1, -1, -1});
        REQUIRE(vec{0, 1, 2, 3}.y() == 1);
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.y(1); return v; }() == vec{-1, 1, -1, -1});
        REQUIRE(vec{0, 1, 2, 3}.z() == 2);
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.z(2); return v; }() == vec{-1, -1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.w() == 3);
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.w(3); return v; }() == vec{-1, -1, -1, 3});
        REQUIRE(vec{0, 1}.xy() == vec{0, 1});
        REQUIRE([](){ auto v = vec2i::filled_with(-1); v.xy(vec{0, 1}); return v; }() == vec{0, 1});
        REQUIRE([](){ auto v = vec2i::filled_with(-1); v.xy(0, 1); return v; }() == vec{0, 1});
        REQUIRE(vec{0, 1}.yx() == vec{1, 0});
        REQUIRE([](){ auto v = vec2i::filled_with(-1); v.yx(vec{1, 0}); return v; }() == vec{0, 1});
        REQUIRE([](){ auto v = vec2i::filled_with(-1); v.yx(1, 0); return v; }() == vec{0, 1});
        REQUIRE(vec{0, 1, 2}.xy() == vec{0, 1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.xy(vec{0, 1}); return v; }() == vec{0, 1, -1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.xy(0, 1); return v; }() == vec{0, 1, -1});
        REQUIRE(vec{0, 1, 2}.xz() == vec{0, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.xz(vec{0, 2}); return v; }() == vec{0, -1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.xz(0, 2); return v; }() == vec{0, -1, 2});
        REQUIRE(vec{0, 1, 2}.yx() == vec{1, 0});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.yx(vec{1, 0}); return v; }() == vec{0, 1, -1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.yx(1, 0); return v; }() == vec{0, 1, -1});
        REQUIRE(vec{0, 1, 2}.yz() == vec{1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.yz(vec{1, 2}); return v; }() == vec{-1, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.yz(1, 2); return v; }() == vec{-1, 1, 2});
        REQUIRE(vec{0, 1, 2}.zx() == vec{2, 0});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.zx(vec{2, 0}); return v; }() == vec{0, -1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.zx(2, 0); return v; }() == vec{0, -1, 2});
        REQUIRE(vec{0, 1, 2}.zy() == vec{2, 1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.zy(vec{2, 1}); return v; }() == vec{-1, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.zy(2, 1); return v; }() == vec{-1, 1, 2});
        REQUIRE(vec{0, 1, 2}.xyz() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.xyz(vec{0, 1, 2}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.xyz(0, 1, 2); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2}.xzy() == vec{0, 2, 1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.xzy(vec{0, 2, 1}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.xzy(0, 2, 1); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2}.yxz() == vec{1, 0, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.yxz(vec{1, 0, 2}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.yxz(1, 0, 2); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2}.yzx() == vec{1, 2, 0});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.yzx(vec{1, 2, 0}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.yzx(1, 2, 0); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2}.zxy() == vec{2, 0, 1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.zxy(vec{2, 0, 1}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.zxy(2, 0, 1); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2}.zyx() == vec{2, 1, 0});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.zyx(vec{2, 1, 0}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.zyx(2, 1, 0); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2, 3}.wx() == vec{3, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wx(vec{3, 0}); return v; }() == vec{0, -1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wx(3, 0); return v; }() == vec{0, -1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.wy() == vec{3, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wy(vec{3, 1}); return v; }() == vec{-1, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wy(3, 1); return v; }() == vec{-1, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.wz() == vec{3, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wz(vec{3, 2}); return v; }() == vec{-1, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wz(3, 2); return v; }() == vec{-1, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.xw() == vec{0, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xw(vec{0, 3}); return v; }() == vec{0, -1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xw(0, 3); return v; }() == vec{0, -1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.xy() == vec{0, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xy(vec{0, 1}); return v; }() == vec{0, 1, -1, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xy(0, 1); return v; }() == vec{0, 1, -1, -1});
        REQUIRE(vec{0, 1, 2, 3}.xz() == vec{0, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xz(vec{0, 2}); return v; }() == vec{0, -1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xz(0, 2); return v; }() == vec{0, -1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.yw() == vec{1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yw(vec{1, 3}); return v; }() == vec{-1, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yw(1, 3); return v; }() == vec{-1, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.yx() == vec{1, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yx(vec{1, 0}); return v; }() == vec{0, 1, -1, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yx(1, 0); return v; }() == vec{0, 1, -1, -1});
        REQUIRE(vec{0, 1, 2, 3}.yz() == vec{1, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yz(vec{1, 2}); return v; }() == vec{-1, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yz(1, 2); return v; }() == vec{-1, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.zw() == vec{2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zw(vec{2, 3}); return v; }() == vec{-1, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zw(2, 3); return v; }() == vec{-1, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zx() == vec{2, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zx(vec{2, 0}); return v; }() == vec{0, -1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zx(2, 0); return v; }() == vec{0, -1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.zy() == vec{2, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zy(vec{2, 1}); return v; }() == vec{-1, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zy(2, 1); return v; }() == vec{-1, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.wxy() == vec{3, 0, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wxy(vec{3, 0, 1}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wxy(3, 0, 1); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.wxz() == vec{3, 0, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wxz(vec{3, 0, 2}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wxz(3, 0, 2); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.wyx() == vec{3, 1, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wyx(vec{3, 1, 0}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wyx(3, 1, 0); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.wyz() == vec{3, 1, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wyz(vec{3, 1, 2}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wyz(3, 1, 2); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.wzx() == vec{3, 2, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wzx(vec{3, 2, 0}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wzx(3, 2, 0); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.wzy() == vec{3, 2, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wzy(vec{3, 2, 1}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wzy(3, 2, 1); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.xwy() == vec{0, 3, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xwy(vec{0, 3, 1}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xwy(0, 3, 1); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.xwz() == vec{0, 3, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xwz(vec{0, 3, 2}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xwz(0, 3, 2); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.xyw() == vec{0, 1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xyw(vec{0, 1, 3}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xyw(0, 1, 3); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.xyz() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xyz(vec{0, 1, 2}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xyz(0, 1, 2); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.xzw() == vec{0, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xzw(vec{0, 2, 3}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xzw(0, 2, 3); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.xzy() == vec{0, 2, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xzy(vec{0, 2, 1}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xzy(0, 2, 1); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.ywx() == vec{1, 3, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ywx(vec{1, 3, 0}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ywx(1, 3, 0); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.ywz() == vec{1, 3, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ywz(vec{1, 3, 2}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ywz(1, 3, 2); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.yxw() == vec{1, 0, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yxw(vec{1, 0, 3}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yxw(1, 0, 3); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.yxz() == vec{1, 0, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yxz(vec{1, 0, 2}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yxz(1, 0, 2); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.yzw() == vec{1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yzw(vec{1, 2, 3}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yzw(1, 2, 3); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.yzx() == vec{1, 2, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yzx(vec{1, 2, 0}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yzx(1, 2, 0); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.zwx() == vec{2, 3, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zwx(vec{2, 3, 0}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zwx(2, 3, 0); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zwy() == vec{2, 3, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zwy(vec{2, 3, 1}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zwy(2, 3, 1); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zxw() == vec{2, 0, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zxw(vec{2, 0, 3}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zxw(2, 0, 3); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zxy() == vec{2, 0, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zxy(vec{2, 0, 1}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zxy(2, 0, 1); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.zyw() == vec{2, 1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zyw(vec{2, 1, 3}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zyw(2, 1, 3); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zyx() == vec{2, 1, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zyx(vec{2, 1, 0}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zyx(2, 1, 0); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.wxyz() == vec{3, 0, 1, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wxyz(vec{3, 0, 1, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wxyz(3, 0, 1, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.wxzy() == vec{3, 0, 2, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wxzy(vec{3, 0, 2, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wxzy(3, 0, 2, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.wyxz() == vec{3, 1, 0, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wyxz(vec{3, 1, 0, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wyxz(3, 1, 0, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.wyzx() == vec{3, 1, 2, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wyzx(vec{3, 1, 2, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wyzx(3, 1, 2, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.wzxy() == vec{3, 2, 0, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wzxy(vec{3, 2, 0, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wzxy(3, 2, 0, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.wzyx() == vec{3, 2, 1, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wzyx(vec{3, 2, 1, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.wzyx(3, 2, 1, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.xwyz() == vec{0, 3, 1, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xwyz(vec{0, 3, 1, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xwyz(0, 3, 1, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.xwzy() == vec{0, 3, 2, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xwzy(vec{0, 3, 2, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xwzy(0, 3, 2, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.xywz() == vec{0, 1, 3, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xywz(vec{0, 1, 3, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xywz(0, 1, 3, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.xyzw() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xyzw(vec{0, 1, 2, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xyzw(0, 1, 2, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.xzwy() == vec{0, 2, 3, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xzwy(vec{0, 2, 3, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xzwy(0, 2, 3, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.xzyw() == vec{0, 2, 1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xzyw(vec{0, 2, 1, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.xzyw(0, 2, 1, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.ywxz() == vec{1, 3, 0, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ywxz(vec{1, 3, 0, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ywxz(1, 3, 0, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.ywzx() == vec{1, 3, 2, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ywzx(vec{1, 3, 2, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ywzx(1, 3, 2, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.yxwz() == vec{1, 0, 3, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yxwz(vec{1, 0, 3, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yxwz(1, 0, 3, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.yxzw() == vec{1, 0, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yxzw(vec{1, 0, 2, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yxzw(1, 0, 2, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.yzwx() == vec{1, 2, 3, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yzwx(vec{1, 2, 3, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yzwx(1, 2, 3, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.yzxw() == vec{1, 2, 0, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yzxw(vec{1, 2, 0, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.yzxw(1, 2, 0, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zwxy() == vec{2, 3, 0, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zwxy(vec{2, 3, 0, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zwxy(2, 3, 0, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zwyx() == vec{2, 3, 1, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zwyx(vec{2, 3, 1, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zwyx(2, 3, 1, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zxwy() == vec{2, 0, 3, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zxwy(vec{2, 0, 3, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zxwy(2, 0, 3, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zxyw() == vec{2, 0, 1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zxyw(vec{2, 0, 1, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zxyw(2, 0, 1, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zywx() == vec{2, 1, 3, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zywx(vec{2, 1, 3, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zywx(2, 1, 3, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.zyxw() == vec{2, 1, 0, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zyxw(vec{2, 1, 0, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.zyxw(2, 1, 0, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1}.gr() == vec{1, 0});
        REQUIRE([](){ auto v = vec2i::filled_with(-1); v.gr(vec{1, 0}); return v; }() == vec{0, 1});
        REQUIRE([](){ auto v = vec2i::filled_with(-1); v.gr(1, 0); return v; }() == vec{0, 1});
        REQUIRE(vec{0, 1}.rg() == vec{0, 1});
        REQUIRE([](){ auto v = vec2i::filled_with(-1); v.rg(vec{0, 1}); return v; }() == vec{0, 1});
        REQUIRE([](){ auto v = vec2i::filled_with(-1); v.rg(0, 1); return v; }() == vec{0, 1});
        REQUIRE(vec{0, 1, 2}.bg() == vec{2, 1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.bg(vec{2, 1}); return v; }() == vec{-1, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.bg(2, 1); return v; }() == vec{-1, 1, 2});
        REQUIRE(vec{0, 1, 2}.br() == vec{2, 0});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.br(vec{2, 0}); return v; }() == vec{0, -1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.br(2, 0); return v; }() == vec{0, -1, 2});
        REQUIRE(vec{0, 1, 2}.gb() == vec{1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.gb(vec{1, 2}); return v; }() == vec{-1, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.gb(1, 2); return v; }() == vec{-1, 1, 2});
        REQUIRE(vec{0, 1, 2}.gr() == vec{1, 0});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.gr(vec{1, 0}); return v; }() == vec{0, 1, -1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.gr(1, 0); return v; }() == vec{0, 1, -1});
        REQUIRE(vec{0, 1, 2}.rb() == vec{0, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.rb(vec{0, 2}); return v; }() == vec{0, -1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.rb(0, 2); return v; }() == vec{0, -1, 2});
        REQUIRE(vec{0, 1, 2}.rg() == vec{0, 1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.rg(vec{0, 1}); return v; }() == vec{0, 1, -1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.rg(0, 1); return v; }() == vec{0, 1, -1});
        REQUIRE(vec{0, 1, 2}.bgr() == vec{2, 1, 0});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.bgr(vec{2, 1, 0}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.bgr(2, 1, 0); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2}.brg() == vec{2, 0, 1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.brg(vec{2, 0, 1}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.brg(2, 0, 1); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2}.gbr() == vec{1, 2, 0});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.gbr(vec{1, 2, 0}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.gbr(1, 2, 0); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2}.grb() == vec{1, 0, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.grb(vec{1, 0, 2}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.grb(1, 0, 2); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2}.rbg() == vec{0, 2, 1});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.rbg(vec{0, 2, 1}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.rbg(0, 2, 1); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2}.rgb() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.rgb(vec{0, 1, 2}); return v; }() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec3i::filled_with(-1); v.rgb(0, 1, 2); return v; }() == vec{0, 1, 2});
        REQUIRE(vec{0, 1, 2, 3}.ab() == vec{3, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ab(vec{3, 2}); return v; }() == vec{-1, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ab(3, 2); return v; }() == vec{-1, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.ag() == vec{3, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ag(vec{3, 1}); return v; }() == vec{-1, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ag(3, 1); return v; }() == vec{-1, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.ar() == vec{3, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ar(vec{3, 0}); return v; }() == vec{0, -1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ar(3, 0); return v; }() == vec{0, -1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.ba() == vec{2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ba(vec{2, 3}); return v; }() == vec{-1, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ba(2, 3); return v; }() == vec{-1, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.bg() == vec{2, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bg(vec{2, 1}); return v; }() == vec{-1, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bg(2, 1); return v; }() == vec{-1, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.br() == vec{2, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.br(vec{2, 0}); return v; }() == vec{0, -1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.br(2, 0); return v; }() == vec{0, -1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.ga() == vec{1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ga(vec{1, 3}); return v; }() == vec{-1, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ga(1, 3); return v; }() == vec{-1, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.gb() == vec{1, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gb(vec{1, 2}); return v; }() == vec{-1, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gb(1, 2); return v; }() == vec{-1, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.gr() == vec{1, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gr(vec{1, 0}); return v; }() == vec{0, 1, -1, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gr(1, 0); return v; }() == vec{0, 1, -1, -1});
        REQUIRE(vec{0, 1, 2, 3}.ra() == vec{0, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ra(vec{0, 3}); return v; }() == vec{0, -1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ra(0, 3); return v; }() == vec{0, -1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.rb() == vec{0, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rb(vec{0, 2}); return v; }() == vec{0, -1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rb(0, 2); return v; }() == vec{0, -1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.rg() == vec{0, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rg(vec{0, 1}); return v; }() == vec{0, 1, -1, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rg(0, 1); return v; }() == vec{0, 1, -1, -1});
        REQUIRE(vec{0, 1, 2, 3}.abg() == vec{3, 2, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.abg(vec{3, 2, 1}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.abg(3, 2, 1); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.abr() == vec{3, 2, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.abr(vec{3, 2, 0}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.abr(3, 2, 0); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.agb() == vec{3, 1, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.agb(vec{3, 1, 2}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.agb(3, 1, 2); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.agr() == vec{3, 1, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.agr(vec{3, 1, 0}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.agr(3, 1, 0); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.arb() == vec{3, 0, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.arb(vec{3, 0, 2}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.arb(3, 0, 2); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.arg() == vec{3, 0, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.arg(vec{3, 0, 1}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.arg(3, 0, 1); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.bag() == vec{2, 3, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bag(vec{2, 3, 1}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bag(2, 3, 1); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.bar() == vec{2, 3, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bar(vec{2, 3, 0}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bar(2, 3, 0); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.bga() == vec{2, 1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bga(vec{2, 1, 3}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bga(2, 1, 3); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.bgr() == vec{2, 1, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bgr(vec{2, 1, 0}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bgr(2, 1, 0); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.bra() == vec{2, 0, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bra(vec{2, 0, 3}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bra(2, 0, 3); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.brg() == vec{2, 0, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.brg(vec{2, 0, 1}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.brg(2, 0, 1); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.gab() == vec{1, 3, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gab(vec{1, 3, 2}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gab(1, 3, 2); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.gar() == vec{1, 3, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gar(vec{1, 3, 0}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gar(1, 3, 0); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.gba() == vec{1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gba(vec{1, 2, 3}); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gba(1, 2, 3); return v; }() == vec{-1, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.gbr() == vec{1, 2, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gbr(vec{1, 2, 0}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gbr(1, 2, 0); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.gra() == vec{1, 0, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gra(vec{1, 0, 3}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gra(1, 0, 3); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.grb() == vec{1, 0, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.grb(vec{1, 0, 2}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.grb(1, 0, 2); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.rab() == vec{0, 3, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rab(vec{0, 3, 2}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rab(0, 3, 2); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.rag() == vec{0, 3, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rag(vec{0, 3, 1}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rag(0, 3, 1); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.rba() == vec{0, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rba(vec{0, 2, 3}); return v; }() == vec{0, -1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rba(0, 2, 3); return v; }() == vec{0, -1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.rbg() == vec{0, 2, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rbg(vec{0, 2, 1}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rbg(0, 2, 1); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.rga() == vec{0, 1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rga(vec{0, 1, 3}); return v; }() == vec{0, 1, -1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rga(0, 1, 3); return v; }() == vec{0, 1, -1, 3});
        REQUIRE(vec{0, 1, 2, 3}.rgb() == vec{0, 1, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rgb(vec{0, 1, 2}); return v; }() == vec{0, 1, 2, -1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rgb(0, 1, 2); return v; }() == vec{0, 1, 2, -1});
        REQUIRE(vec{0, 1, 2, 3}.abgr() == vec{3, 2, 1, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.abgr(vec{3, 2, 1, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.abgr(3, 2, 1, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.abrg() == vec{3, 2, 0, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.abrg(vec{3, 2, 0, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.abrg(3, 2, 0, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.agbr() == vec{3, 1, 2, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.agbr(vec{3, 1, 2, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.agbr(3, 1, 2, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.agrb() == vec{3, 1, 0, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.agrb(vec{3, 1, 0, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.agrb(3, 1, 0, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.arbg() == vec{3, 0, 2, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.arbg(vec{3, 0, 2, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.arbg(3, 0, 2, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.argb() == vec{3, 0, 1, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.argb(vec{3, 0, 1, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.argb(3, 0, 1, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.bagr() == vec{2, 3, 1, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bagr(vec{2, 3, 1, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bagr(2, 3, 1, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.barg() == vec{2, 3, 0, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.barg(vec{2, 3, 0, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.barg(2, 3, 0, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.bgar() == vec{2, 1, 3, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bgar(vec{2, 1, 3, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bgar(2, 1, 3, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.bgra() == vec{2, 1, 0, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bgra(vec{2, 1, 0, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.bgra(2, 1, 0, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.brag() == vec{2, 0, 3, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.brag(vec{2, 0, 3, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.brag(2, 0, 3, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.brga() == vec{2, 0, 1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.brga(vec{2, 0, 1, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.brga(2, 0, 1, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.gabr() == vec{1, 3, 2, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gabr(vec{1, 3, 2, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gabr(1, 3, 2, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.garb() == vec{1, 3, 0, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.garb(vec{1, 3, 0, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.garb(1, 3, 0, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.gbar() == vec{1, 2, 3, 0});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gbar(vec{1, 2, 3, 0}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gbar(1, 2, 3, 0); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.gbra() == vec{1, 2, 0, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gbra(vec{1, 2, 0, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.gbra(1, 2, 0, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.grab() == vec{1, 0, 3, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.grab(vec{1, 0, 3, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.grab(1, 0, 3, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.grba() == vec{1, 0, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.grba(vec{1, 0, 2, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.grba(1, 0, 2, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.rabg() == vec{0, 3, 2, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rabg(vec{0, 3, 2, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rabg(0, 3, 2, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.ragb() == vec{0, 3, 1, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ragb(vec{0, 3, 1, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.ragb(0, 3, 1, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.rbag() == vec{0, 2, 3, 1});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rbag(vec{0, 2, 3, 1}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rbag(0, 2, 3, 1); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.rbga() == vec{0, 2, 1, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rbga(vec{0, 2, 1, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rbga(0, 2, 1, 3); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.rgab() == vec{0, 1, 3, 2});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rgab(vec{0, 1, 3, 2}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rgab(0, 1, 3, 2); return v; }() == vec{0, 1, 2, 3});
        REQUIRE(vec{0, 1, 2, 3}.rgba() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rgba(vec{0, 1, 2, 3}); return v; }() == vec{0, 1, 2, 3});
        REQUIRE([](){ auto v = vec4i::filled_with(-1); v.rgba(0, 1, 2, 3); return v; }() == vec{0, 1, 2, 3});
    }
}


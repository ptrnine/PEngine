#include <catch2/catch.hpp>
#include <core/print.hpp>
#include <core/serialization.hpp>
#include <core/vec.hpp>

TEST_CASE("serialization & deserialization") {
    using namespace core;

    SECTION("basic types") {
        byte_vector out;
        serialize(uint32_t(256), out);
        REQUIRE(format("{}", out) == "{ 0x00, 0x01, 0x00, 0x00 }");
        {
            int a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(a == 256);
        }
        out.clear();

        serialize(200.200f, out);
        REQUIRE(format("{}", out) == "{ 0x33, 0x33, 0x48, 0x43 }");
        {
            float a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(a == 200.200f);
        }
        out.clear();

        serialize(false, out);
        REQUIRE(format("{}", out) == "{ 0x00 }");
        {
            bool a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(!a);
        }
        out.clear();


        serialize(1.0, out);
        REQUIRE(format("{}", out) == "{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f }");
        {
            double a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(a == 1.0);
        }
        out.clear();

        serialize(optional<int>(), out);
        REQUIRE(format("{}", out) == "{ 0x00 }");
        {
            optional<int> a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(!a.has_value());
        }
        out.clear();

        serialize(optional<double>(1.0), out);
        REQUIRE(format("{}", out) == "{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x3f }");
        {
            optional<double> a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(a.has_value());
            REQUIRE(a.value() == 1.0);
        }
        out.clear();

        // Todo: fix for 32-bit platforms

        serialize(string("abc"), out);
        REQUIRE(format("{}", out) == "{ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x61, 0x62, 0x63 }");
        {
            string a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(a == "abc");
        }
        out.clear();

        serialize(vector{1, 2, 3}, out);
        REQUIRE(format("{}", out) == "{ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,"
                                     " 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 }");
        {
            vector<int> a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(a == vector{1, 2, 3});
        }
        out.clear();

        serialize(tuple{1, true, optional<double>(228.0)}, out);
        REQUIRE(format("{}", out) == "{ 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, "
                                     "0x00, 0x00, 0x00, 0x80, 0x6c, 0x40 }");
        {
            tuple<int, bool, optional<double>> a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(a == tuple{1, true, optional<double>(228.0)});
        }
        out.clear();

        serialize(pair{1, "kek"s}, out);
        REQUIRE(format("{}", out) == "{ 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, "
                                     "0x00, 0x00, 0x00, 0x00, 0x00, 0x6b, 0x65, 0x6b }");
        {
            pair<int, string> a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(a == pair{1, "kek"s});
        }
        out.clear();

        serialize(vec{1, 2, 3}, out);
        REQUIRE(format("{}", out) == "{ 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00 }");
        {
            vec3i a;
            deserializer_view ds(out);
            ds.read(a);
            REQUIRE(a == vec{1, 2, 3});
        }
        out.clear();
    }

    SECTION("class member serialization") {
        struct test_class {
        public:
            EVO_SERIALIZE(a, b, c, d)

            void clear() {
                a = 0;
                b = {};
                c = 0;
                d.clear();
            }

            int a = 228;
            tuple<optional<int>, array<int, 2>> b = {1, {1, 2}};
            double c = 33432.023;
            vector<float> d = { 1.2f, 2.2f };
        };

        serializer s;
        test_class t;
        s.write(t);
        REQUIRE(format("{}", s.data()) == "{ 0xe4, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, "
                                          "0x00, 0x02, 0x00, 0x00, 0x00, 0xfa, 0x7e, 0x6a, 0xbc, 0x00, 0x53, 0xe0, 0x40, "
                                          "0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x9a, 0x99, 0x99, 0x3f, 0xcd, "
                                          "0xcc, 0x0c, 0x40 }");

        t.clear();
        byte_vector buff = s.data();
        deserializer_view ds(buff);
        ds.read(t);
        REQUIRE(t.a == test_class().a);
        REQUIRE(t.b == test_class().b);
        REQUIRE(t.c == test_class().c);
        REQUIRE(t.d == test_class().d);
    }
}
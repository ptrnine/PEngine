#include <catch2/catch.hpp>
#include <core/print.hpp>
#include <core/serialization.hpp>
#include <core/vec.hpp>

struct some_type {
    core::pair<core::string, int> a;
    double b;
};

namespace core {
    template <>
    struct pe_serialize<some_type> {
        void operator()(const some_type& s, byte_vector& v) {
            serialize(s.a, v);
            serialize(s.b, v);
        }
    };
    template <>
    struct pe_deserialize<some_type> {
        void operator()(some_type& s, span<const byte>& v) {
            deserialize(s.a, v);
            deserialize(s.b, v);
        }
    };

}

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
            PE_SERIALIZE(a, b, c, d)

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

    SECTION("map serialization") {
        hash_map<string, double> map;
        map.emplace("one", 1.111);
        map.emplace("two", 2.222);

        serializer s;
        s.write(map);
        auto s1 = format("{}", s.data());
        bool cmp =
            s1 == "{ 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, "
                  "0x00, 0x00, 0x00, 0x00, 0x6f, 0x6e, 0x65, 0x2d, 0xb2, 0x9d, 0xef, 0xa7, "
                  "0xc6, 0xf1, 0x3f, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, "
                  "0x77, 0x6f, 0x2d, 0xb2, 0x9d, 0xef, 0xa7, 0xc6, 0x01, 0x40 }" ||
            s1 == "{ 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x77, 0x6f, 0x2d, "
                  "0xb2, 0x9d, 0xef, 0xa7, 0xc6, 0x01, 0x40, 0x02, 0x00, 0x00, 0x00, 0x00, "
                  "0x00, 0x00, 0x00, 0x03, 0x00, 0x0      0, 0x00, 0x00, 0x00, 0x00, 0x00, "
                  "0x6f, 0x6e, 0x65, 0x2d, 0xb2, 0x9d, 0xef, 0xa7, 0xc6, 0xf1, 0x3f }";
        REQUIRE(cmp);

        hash_map<string, double> map2;
        auto bytes = s.data();
        auto ds = deserializer_view(bytes);
        ds.read(map2);

        REQUIRE(map == map2);
    }

    SECTION("external functor serialization") {
        some_type v;
        v.a = pair{string("string"), 228};
        v.b = 228.228;

        serializer s;
        s.write(v);

        REQUIRE(format("{}", s.data()) ==
                "{ 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x74, 0x72, 0x69, 0x6e, "
                "0x67, 0xe4, 0x00, 0x00, 0x00, 0x9e, 0xef, 0xa7, 0xc6, 0x4b, 0x87, 0x6c, 0x40 }");

        some_type v2;
        auto bytes = s.data();
        auto ds = deserializer_view(bytes);
        ds.read(v2);

        REQUIRE(v.a == v2.a);
        REQUIRE(std::memcmp(&v.b, &v2.b, sizeof(v.b)) == 0);
    }
}

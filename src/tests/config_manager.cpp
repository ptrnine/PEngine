#include <catch2/catch.hpp>
#include <core/vec.hpp>
#include <core/config_manager.hpp>

TEST_CASE("config_manager") {
    using namespace core;

    SECTION("Config cast") {
        REQUIRE(config_cast<string>("value") == "value");
        REQUIRE(config_cast<string_view>("\"value\"") == "value");
        REQUIRE(config_cast<const char*>("\"{value}\"") == "{value}");

        REQUIRE(config_cast<uint8_t>("244") == 244);
        REQUIRE(config_cast<int8_t> ("-100") == -100);
        REQUIRE(config_cast<uint16_t>("13666") == 13666);
        REQUIRE(config_cast<int16_t> ("-7000") == -7000);
        REQUIRE(config_cast<uint32_t>("4294967295") == 4294967295);
        REQUIRE(config_cast<int32_t> ("-2147483646") == -2147483646);
        REQUIRE(config_cast<uint64_t>("18446744073709551615") == 18446744073709551615ull);
        REQUIRE(config_cast<int64_t> ("-9223372036854775807") == -9223372036854775807ll);

        REQUIRE(config_cast<array<int, 5>>("1, 2, 3, 4, 5") == array{1, 2, 3, 4, 5});
        REQUIRE(config_cast<vector<bool>>("{ true, on, false, off }") == vector{true, true, false, false});
        REQUIRE(config_cast<pair<int, float>>("1, 4.4") == pair{1, 4.4f});
        REQUIRE(config_cast<tuple<bool, double, string>>("true, 4.4, string") == tuple{true, 4.4, "string"});

        REQUIRE(config_cast<hash_map<string, int>>("{one, 1}, {two, 2}, {three, 3}") == hash_map<string, int>{{"one", 1}, {"two", 2}, {"three", 3}});
        REQUIRE(config_cast<vector<tuple<int, pair<bool, bool>>>>("{1, {true, true}}, {2, {false, off}}") == vector<tuple<int, pair<bool, bool>>>{ {1, {true, true}}, {2, {false, false}}});
    }

    SECTION("Config reading") {
        config_manager m(cfg_reentry("test", ::platform_dependent::get_exe_dir() / "../../../fs.cfg"));
        REQUIRE(vector{1, 2, 3, 4, 5} == m.read_unwrap<vector<int>>("c", "nums"));
        REQUIRE("1, 2, 3, 4, 5" == m.read_unwrap<string>("c", "nums_str"));
        REQUIRE("1, 2, 3, 4, 5" == m.read_unwrap<string>("c", "nums_str2"));
        REQUIRE(m.read_default("nine", 9) == 9);
        REQUIRE(m.read_default("one", 1) == 11);
    }

    SECTION("Class test") {
        config_manager m(cfg_reentry("test", ::platform_dependent::get_exe_dir() / "../../../fs.cfg"));

        class test_class {
        public:
            test_class(config_manager& m) {
                m.read_cfg_values(val1, val2, val3);
            }

            CFG_VAL(int,    "test_class", val1);
            CFG_VAL(string, "test_class", val2);
            CFG_VAL(double, "test_class", val3);
        };

        test_class t(m);

        REQUIRE(t.val1.value() == 1);
        REQUIRE(t.val2.value() == "value");
        REQUIRE(essentially_equal(t.val3.value(), 228.228, 0.001));
    }

    SECTION("Direct reading") {
        auto sect = config_section::direct_read("direct_test",
                cfg_reentry("test", ::platform_dependent::get_exe_dir() / "../../../fs.cfg"));
        REQUIRE(vector{1, 2, 3, 4, 5} == sect.read_unwrap<vector<int>>("nums"));
        REQUIRE("its a string" == sect.read_unwrap<string>("str"));
        REQUIRE(essentially_equal(sect.read_unwrap<float>("float"), 228.228f, 0.001f));
        REQUIRE(tuple{true, string("string"), vec3i{1, 2, 3}} == sect.read_unwrap<tuple<bool, string, vec3i>>("multi"));
    }
}

#include "core/config_manager.hpp"
#include <catch2/catch.hpp>
#include <core/views/split.hpp>
#include <core/views/drop.hpp>
#include <core/views/skip.hpp>
#include <core/views/sub.hpp>
#include <core/acts/drop.hpp>
#include <core/acts/drop_pref.hpp>
#include <core/acts/trim.hpp>
#include <core/acts/to.hpp>
#include <core/acts/unbracket.hpp>
#include <core/range_op.hpp>

using namespace core;
namespace vs = core::views;
namespace as = core::acts;

TEST_CASE("ranges") {
    SECTION("views::split") {
        REQUIRE("a;b;c"sv / vs::split(';') / as::to<vector<string_view>>() ==
                vector{"a"sv, "b"sv, "c"sv});
        REQUIRE("a;b;c,ddd;,e"sv / vs::split(';', ',') / as::to<vector<string_view>>() ==
                vector{"a"sv, "b"sv, "c"sv, "ddd"sv, "e"sv});
        REQUIRE("a;b;c,ddd;,e"sv / vs::split(vs::split_mode::allow_empty, ';', ',') /
                    as::to<vector<string_view>>() ==
                vector{"a"sv, "b"sv, "c"sv, "ddd"sv, ""sv, "e"sv});
        REQUIRE(";"sv / vs::split(';') / as::to<vector<string_view>>() == vector<string_view>());
        REQUIRE(""sv / vs::split(';') / as::to<vector<string_view>>() == vector<string_view>());
        REQUIRE(";"sv / vs::split(vs::split_mode::allow_empty, ';') /
                    as::to<vector<string_view>>() ==
                vector{""sv});
        REQUIRE(""sv / vs::split(vs::split_mode::allow_empty, ';') /
                    as::to<vector<string_view>>() ==
                vector<string_view>());
        REQUIRE(";a;b;c"sv / vs::split(';') / as::to<vector<string_view>>() ==
                vector{"a"sv, "b"sv, "c"sv});
        REQUIRE(";a;b;c;"sv / vs::split(';') / as::to<vector<string_view>>() ==
                vector{"a"sv, "b"sv, "c"sv});
        REQUIRE(";;;;;a;;;;b;;;;c;;;"sv / vs::split(';') / as::to<vector<string_view>>() ==
                vector{"a"sv, "b"sv, "c"sv});

        REQUIRE(";a;b,c"sv / vs::split(vs::split_mode::allow_empty, ';', ',') /
                    as::to<vector<string_view>>() ==
                vector{""sv, "a"sv, "b"sv, "c"sv});
        REQUIRE(";a;b;c,"sv / vs::split(vs::split_mode::allow_empty, ';', ',') /
                    as::to<vector<string_view>>() ==
                vector{""sv, "a"sv, "b"sv, "c"sv});
        REQUIRE(";,a;;b;;c,;"sv / vs::split(vs::split_mode::allow_empty, ';', ',') /
                    as::to<vector<string_view>>() ==
                vector{""sv, ""sv, "a"sv, ""sv, "b"sv, ""sv, "c"sv, ""sv});
    }

    SECTION("views::drop") {
        REQUIRE("mda;lol"sv / vs::drop_after(';') / as::to<string>() == "mda"sv);
        REQUIRE(";mdalol"sv / vs::drop_after(';') / as::to<string>() == ""sv);
        REQUIRE("mdalol;"sv / vs::drop_after(';') / as::to<string>() == "mdalol"sv);
        REQUIRE("mda//lol"sv / vs::drop_after("//"sv) / as::to<string>() == "mda"sv);
        REQUIRE("//mdalol"sv / vs::drop_after("//"sv) / as::to<string>() == ""sv);
        REQUIRE("mdalol//"sv / vs::drop_after("//"sv) / as::to<string>() == "mdalol"sv);

        REQUIRE("mda;lol"sv / vs::drop_when(oneline_comment_start()) / as::to<string>() == "mda"sv);
        REQUIRE("mda//lol"sv / vs::drop_when(oneline_comment_start()) / as::to<string>() ==
                "mda"sv);
        REQUIRE(";mdalol"sv / vs::drop_when(oneline_comment_start()) / as::to<string>() == ""sv);
        REQUIRE("//mdalol"sv / vs::drop_when(oneline_comment_start()) / as::to<string>() == ""sv);
        REQUIRE("mdalol;"sv / vs::drop_when(oneline_comment_start()) / as::to<string>() ==
                "mdalol"sv);
        REQUIRE("mdalol//"sv / vs::drop_when(oneline_comment_start()) / as::to<string>() ==
                "mdalol"sv);
    }

    SECTION("views::skip") {
        REQUIRE("kek;lol;"sv / vs::skip_when([](char c) { return c == ';'; }) / as::to<string>() ==
                "keklol"sv);
        REQUIRE(";;;k;;e;;k;l;;o;;l;;;"sv / vs::skip_when([](char c) { return c == ';'; }) /
                    as::to<string>() ==
                "keklol"sv);
        REQUIRE(";;;;;;"sv / vs::skip_when([](char c) { return c == ';'; }) / as::to<string>() ==
                ""sv);
        REQUIRE("abc'1234'de'56'f"sv / vs::skip_when(exclude_in_quotes()) / as::to<string>() ==
                "abcdef"sv);
        REQUIRE("abc'1234'de\"56\"f"sv / vs::skip_when(exclude_in_quotes()) / as::to<string>() ==
                "abcdef"sv);
        REQUIRE("abc'1\"23\"4'de'56'f"sv / vs::skip_when(exclude_in_quotes()) / as::to<string>() ==
                "abcdef"sv);
        REQUIRE("abc'1\"23\"4'de\"5'6'\"f"sv / vs::skip_when(exclude_in_quotes()) /
                    as::to<string>() ==
                "abcdef"sv);
    }

    SECTION("views::sub") {
        REQUIRE("kek   ;    lol    ; mda"sv / vs::split(';') / vs::sub(as::trim(' ')) /
                    as::to<vector<string_view>>() ==
                vector{"kek"sv, "lol"sv, "mda"sv});
        REQUIRE("a,b,c   ;   d,e,f   ; g"sv / vs::split(';') / vs::sub(as::trim(' ')) /
                    vs::sub(vs::split(',')) / as::to<vector<vector<string_view>>>() ==
                vector{vector{"a"sv, "b"sv, "c"sv}, vector{"d"sv, "e"sv, "f"sv}, vector{"g"sv}});
    }

    SECTION("acts::drop") {
        REQUIRE("mda;lol"sv / as::drop_when(oneline_comment_start()) / as::to<string_view>() ==
                "mda"sv);
        REQUIRE("mda//lol"sv / as::drop_when(oneline_comment_start()) / as::to<string_view>() ==
                "mda"sv);
        REQUIRE(";mdalol"sv / as::drop_when(oneline_comment_start()) / as::to<string_view>() ==
                ""sv);
        REQUIRE("//mdalol"sv / as::drop_when(oneline_comment_start()) / as::to<string_view>() ==
                ""sv);
        REQUIRE("mdalol;"sv / as::drop_when(oneline_comment_start()) / as::to<string_view>() ==
                "mdalol"sv);
        REQUIRE("mdalol//"sv / as::drop_when(oneline_comment_start()) / as::to<string_view>() ==
                "mdalol"sv);
    }

    SECTION("acts::drop_pref") {
        REQUIRE("kek_lol"sv / as::drop_pref("kek_"sv) / as::to<string_view>() == "lol"sv);
        REQUIRE("kek_lol"sv / as::drop_pref(vector{'k', 'e', 'k', '_'}) / as::to<string_view>() ==
                "lol"sv);
        REQUIRE("klol"sv / as::drop_pref('k') / as::to<string_view>() == "lol"sv);
    }

    SECTION("acts::trim") {
        REQUIRE("kek"sv / as::trim(' ', '\t') / as::to<string_view>() == "kek"sv);
        REQUIRE("    \t \t \tk e\tk"sv / as::trim(' ', '\t') / as::to<string_view>() == "k e\tk"sv);
        REQUIRE("    \t \t \tk\te k \t \t  "sv / as::trim(' ', '\t') / as::to<string_view>() == "k\te k"sv);

        REQUIRE("aaakek"sv / as::trim_when([](char c) { return c == 'a' || c == 'b'; }) / as::to<string_view>() == "kek"sv);
        REQUIRE("kek"sv / as::trim_when([](char c) { return c == 'a' || c == 'b'; }) / as::to<string_view>() == "kek"sv);
        REQUIRE("kekaabb"sv / as::trim_when([](char c) { return c == 'a' || c == 'b'; }) / as::to<string_view>() == "kek"sv);
        REQUIRE("aababbakeaaakaabb"sv / as::trim_when([](char c) { return c == 'a' || c == 'b'; }) / as::to<string_view>() == "keaaak"sv);
    }

    SECTION("acts::unbracket") {
        REQUIRE("lol"sv / as::unbracket('\'', '\'') / as::to<string_view>() == "lol"sv);
        REQUIRE("\'lol"sv / as::unbracket('\'', '\'') / as::to<string_view>() == "\'lol"sv);
        REQUIRE("lol\'"sv / as::unbracket('\'', '\'') / as::to<string_view>() == "lol\'"sv);
        REQUIRE("\'lol\'"sv / as::unbracket('\'', '\'') / as::to<string_view>() == "lol"sv);

        REQUIRE("lol"sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "lol"sv);
        REQUIRE("\"lol"sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "\"lol"sv);
        REQUIRE("lol\""sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "lol\""sv);
        REQUIRE("lol\""sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "lol\""sv);
        REQUIRE("'lol\""sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "'lol\""sv);
        REQUIRE("\"lol'"sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "\"lol'"sv);
        REQUIRE("\"lol\""sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "lol"sv);
        REQUIRE("'lol'"sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "lol"sv);
        REQUIRE("\"'lol'\""sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "'lol'"sv);
        REQUIRE("'\"lol\"'"sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "\"lol\""sv);

        REQUIRE(""sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == ""sv);
        REQUIRE("\"\""sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == ""sv);
        REQUIRE("''"sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == ""sv);
        REQUIRE("\"'"sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "\"'"sv);
        REQUIRE("'\""sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "'\""sv);
        REQUIRE("\""sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "\""sv);
        REQUIRE("'"sv / as::unbracket(array{'\'', '"'}, array{'\'', '"'}) / as::to<string_view>() == "'"sv);
    }
}

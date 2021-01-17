#include <catch2/catch.hpp>
#include <core/serialization.hpp>
#include <core/print.hpp>
#include <util/compression.hpp>

using namespace core;
using namespace util;

TEST_CASE("Compression") {
    string data = "afdadwqdcopeewdpoekdopwkfioejguefoakekf3rio3jrewakdeawodkeofpaekr348rjwakdepakde"
                  "waokdewddwawwwwwwwwwwwwwqopeqweopqpdwokqowpdkqwopeiqpi3o2i3poq2ieoqpdq2op3pq2okd"
                  "opwqkop23kqopkwdpoqkpo23kpoqekpwkqpo32";

    constexpr size_t repeats = 10;

    for (size_t i = 0; i < repeats; ++i) {
        serializer s;
        s.write(data);

        auto deflated = compress_block(s.data());
        auto inflated = decompress_block(deflated, s.data().size());
        REQUIRE(s.data() == inflated);

        deflated = compress_block(
            span(const_cast<byte*>(s.data().data()), static_cast<ssize_t>(s.data().size())));
        inflated = decompress_block(deflated, s.data().size());
        REQUIRE(s.data() == inflated);

        deflated = compress(s.data());
        inflated = decompress(deflated);
        REQUIRE(s.data() == inflated);

        deflated = compress_block(s.data());
        inflated = decompress(deflated);
        REQUIRE(s.data() == inflated);

        deflated = compress(s.data());
        inflated = decompress_block(deflated, s.data().size());
        REQUIRE(s.data() == inflated);

        data += data;
    }
}

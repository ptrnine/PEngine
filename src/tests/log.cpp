#include <catch2/catch.hpp>
#include <core/log.hpp>
#include <core/async.hpp>

using namespace core;

/* TODO: rewrite this */
#if 0

TEST_CASE("Log multithread test") {
    auto ss = make_shared<std::stringstream>();
    logger::instance().remove_output_stream("stdout");
    logger::instance().add_output_stream("string stream", ss);

    LOG("Hello");
    REQUIRE(ss->str() == ": Hello\n");
    ss->str("");

    constexpr size_t copies_count = 10;
    constexpr size_t nprocs = 16;
    constexpr size_t iterations = 1000;

    std::string checkstr = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for (size_t i = 0; i < copies_count; ++i)
        checkstr += checkstr;

    std::string result_str;
    for (size_t i = 0; i < nprocs; ++i)
        result_str += ": " + checkstr + "\n";

    auto logwrite = [&] { LOG(checkstr); };


    for (size_t i = 0; i < iterations; ++i) {
        array<future<void>, nprocs> futures;
        for (auto& f : futures)
            f = async_call(logwrite);

        for (auto& f : futures)
            f.wait();

        REQUIRE(ss->str() == result_str);
        ss->str("");
    }
}

#endif

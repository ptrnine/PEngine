#include <core/async.hpp>
#include <core/time.hpp>
#include <catch2/catch.hpp>

using namespace core;
using namespace core::chrono_literals;

struct test {
    test() {
        throw std::runtime_error("kek");
    }
    test(int val) {
        v = val;
    }
    int v = 0;
};


TEST_CASE("async") {
    SECTION("deffered_resource") {
        deffered_resource<future<test>> res;

        REQUIRE(!res.is_ready());
        REQUIRE(res.empty());

        auto& opt = res.try_get();
        REQUIRE(!opt);

        bool except = false;
        try {
            opt.value();
        } catch (...) {
            except = true;
        }
        REQUIRE(except);

        except = false;
        try {
            [[maybe_unused]]
            auto& v = res.get();
        } catch (...) {
            except = true;
        }
        REQUIRE(except);

        res = async_call([](){ std::this_thread::sleep_for(10ms); return test(228); });
        REQUIRE(!res.empty());
        res = async_call([](){ std::this_thread::sleep_for(100ms); return test(1337); });

        // Increase latency if it fails
        REQUIRE(res.is_ready() == false);

        REQUIRE(res.get().v == 1337);
        REQUIRE(res.is_ready());
        REQUIRE(!res.empty());

        res = async_call([]() { return test(); });
        except = false;
        try {
            [[maybe_unused]]
            auto& val = res.try_get().value();
        } catch (...) {
            except = true;
        }
        REQUIRE(except);
    }
}


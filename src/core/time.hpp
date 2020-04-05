#pragma once

#include <chrono>
#include "types.hpp"


namespace core {
    using std::chrono::time_point;
    using std::chrono::duration;
    using std::chrono::steady_clock;
    using std::chrono::system_clock;
    using std::chrono::nanoseconds;
    using std::chrono::microseconds;
    using std::chrono::milliseconds;
    using std::chrono::seconds;
    using std::chrono::minutes;
    using std::chrono::hours;
    using std::chrono::duration_cast;
    using namespace std::chrono_literals;

    class timer {
    public:
        [[nodiscard]]
        auto measure() const {
            return steady_clock::now() - _start;
        }

        template <typename T>
        [[nodiscard]]
        auto measure() const {
            return duration_cast<T>(steady_clock::now() - _start).count();
        }

        void reset() {
            _start = steady_clock::now();
        }

    private:
        time_point<steady_clock> _start = steady_clock::now();
    };
}
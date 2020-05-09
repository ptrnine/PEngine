#pragma once

#include <chrono>
#include "helper_macros.hpp"
#include "types.hpp"


namespace core {
    using std::chrono::time_point;
    using std::chrono::duration;
    using std::chrono::steady_clock;
    using std::chrono::system_clock;

    using nanoseconds  = std::ratio<1, 1000000000>;
    using microseconds = std::ratio<1, 1000000>;
    using milliseconds = std::ratio<1, 1000>;
    using seconds      = std::ratio<1, 1>;
    using minutes      = std::ratio<60>;
    using hours        = std::ratio<3600>;
    using std::chrono::duration_cast;
    using namespace std::chrono_literals;

    class timer {
    public:
        template <typename T = double, typename Ratio = seconds>
        [[nodiscard]]
        auto measure() const {
            return duration_cast<duration<T, Ratio>>(steady_clock::now() - _start).count();
        }

        void reset() {
            _start = steady_clock::now();
        }

        template <typename T = double, typename Ratio = seconds>
        T tick() {
            auto now = steady_clock::now();
            T result = duration_cast<duration<T, Ratio>>(now - _start).count();
            _start = now;
            return result;
        }

    private:
        time_point<steady_clock> _start = steady_clock::now();
    };

    class st_global_timer {
        SINGLETON_IMPL(st_global_timer);

    public:
        timer _timer;
        st_global_timer() = default;
    };

    inline timer& global_timer() {
        return st_global_timer::instance()._timer;
    }
}
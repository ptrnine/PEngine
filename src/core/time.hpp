#pragma once

#include <chrono>
#include "helper_macros.hpp"
#include "types.hpp"


namespace core {
    using std::chrono::time_point;
    using std::chrono::duration;
    using std::chrono::steady_clock;
    using std::chrono::system_clock;

    using nanoseconds  = chrono::nanoseconds;  //std::ratio<1, 1000000000>;
    using microseconds = chrono::microseconds; //std::ratio<1, 1000000>;
    using milliseconds = chrono::milliseconds; //std::ratio<1, 1000>;
    using seconds      = chrono::seconds;      //std::ratio<1, 1>;
    using minutes      = chrono::minutes;      //std::ratio<60>;
    using hours        = chrono::hours;        //std::ratio<3600>;
    using std::chrono::duration_cast;
    using namespace std::chrono_literals;

    class timer {
    public:
        template <typename T = double, typename Period = seconds::period>
        [[nodiscard]]
        auto measure_count() const {
            return duration_cast<duration<T, Period>>(steady_clock::now() - _start).count();
        }

        [[nodiscard]]
        auto measure() const {
            return steady_clock::now() - _start;
        }

        void reset() {
            _start = steady_clock::now();
        }

        template <typename T = double, typename Period = seconds::period>
        T tick_count() {
            auto now = steady_clock::now();
            T result = duration_cast<duration<T, Period>>(now - _start).count();
            _start = now;
            return result;
        }

        auto tick() {
            auto now = steady_clock::now();
            auto res = now - _start;
            _start = now;
            return res;
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

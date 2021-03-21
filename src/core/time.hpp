#pragma once

#include <chrono>
#include <iomanip>
#include "helper_macros.hpp"
#include "types.hpp"
#include "platform_dependent.hpp"

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

    namespace details {
        template <typename T>
        struct is_ratio : std::false_type {};

        template <intmax_t Num, intmax_t Den>
        struct is_ratio<std::ratio<Num, Den>> : std::true_type {};
    }
    template <typename T>
    concept Ratio = details::is_ratio<T>::value;

    namespace details {
        template <typename T>
        struct is_duration : std::false_type {};

        template <typename T, Ratio R>
        struct is_duration<core::duration<T, R>> : std::true_type {};
    }
    template <typename T>
    concept Duration = details::is_duration<T>::value;

    template <typename T = double>
    T duration_to_float_seconds(Duration auto iduration) {
        return duration_cast<duration<T, std::ratio<1, 1>>>(iduration).count();
    }

    class timer {
    public:
        template <typename T = double, typename Period = seconds::period>
        [[nodiscard]]
        auto measure_count() const {
            return duration_cast<duration<T, Period>>(steady_clock::now() - _start).count();
        }

        template <typename T = nanoseconds>
        [[nodiscard]]
        auto measure() const {
            return duration_cast<T>(steady_clock::now() - _start);
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

        template <typename T = nanoseconds>
        auto tick() {
            auto now = steady_clock::now();
            auto res = duration_cast<T>(now - _start);
            _start = now;
            return res;
        }

        template <typename T, typename Period>
        void add_to_timepoint(const duration<T, Period>& duration) {
            _start += duration;
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

    inline string current_datetime(string_view format) {
        std::stringstream ss;

        auto now = std::chrono::system_clock::now();
        auto now_tt = std::chrono::system_clock::to_time_t(now);
        auto now_tm = platform_dependent::localtime(now_tt);
        auto ns = duration_cast<nanoseconds>(now - std::chrono::system_clock::from_time_t(now_tt))
                      .count();
        auto us = ns / 1000;
        auto ms = us / 1000;

        array<bool, 256> charmap = {false};
        charmap['D'] = charmap['M'] = charmap['Y'] = charmap['h'] = charmap['m'] = charmap['s'] =
            charmap['x'] = charmap['u'] = charmap['n'] = true;

        static constexpr auto padprint = [](std::stringstream& ss, int c, auto v) {
            ss << std::setfill('0') << std::setw(c) << v;
        };

        int counter = 0;

        auto printtm = [&](char f) {
            if (counter) {
                switch (f) {
                case 'D': padprint(ss, counter, now_tm.tm_mday); break;
                case 'M': padprint(ss, counter, now_tm.tm_mon + 1); break;
                case 'Y': padprint(ss, counter, now_tm.tm_year + 1900); break;
                case 'h': padprint(ss, counter, now_tm.tm_hour); break;
                case 'm': padprint(ss, counter, now_tm.tm_min); break;
                case 's': padprint(ss, counter, now_tm.tm_sec); break;
                case 'x': padprint(ss, counter, ms); break;
                case 'u': padprint(ss, counter, us); break;
                case 'n': padprint(ss, counter, ns); break;
                default: break;
                }
                counter = 0;
            }
        };

        char last_fmt = '\0';
        for (auto c : format) {
            auto is_fmt = charmap[static_cast<u8>(c)];

            if (!is_fmt || last_fmt != c)
                printtm(last_fmt);

            if (is_fmt)
                ++counter;
            else {
                ss << c;
                counter = 0;
            }

            if (is_fmt && last_fmt != c)
                last_fmt = c;
        }
        printtm(last_fmt);

        return ss.str();
    }
}

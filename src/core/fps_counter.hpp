#pragma once

#include <thread>

#include "time.hpp"
#include "types.hpp"

namespace core {
    class fps_counter {
    public:
        void update() {
            if (accumulator++ == accumulator_max - 1) {
                fps = static_cast<double>(accumulator_max) / timer.tick();
                accumulator = 0;
            }
        }

        double get() {
            return fps;
        }

    private:
        double fps = 0;
        size_t accumulator = 0;
        size_t accumulator_max = 10;
        core::timer timer;
    };

    class update_smoother {
    public:
        update_smoother(size_t frames_count = 10, double ahead_factor = 0.8):
                counter_max(frames_count), ahead_fact(ahead_factor) {}

        void smooth() {
            if (reset_step < reset_timer.measure()) {
                reset_timer.reset();
                ahead_shift = 0;
                average_step = 1 / 10000.0;
                counter = 0;
            }

            if (counter == counter_max - 1) {
                ahead_shift = delayed_sum / static_cast<double>(counter_max);
                average_step = timer.tick() / static_cast<double>(counter_max);
                delayed_sum = 0.0;
            }

            ++counter;
            if (counter >= counter_max)
                counter = 0;

            if (delayer.measure() > average_step)
                delayed_sum += delayer.measure() - average_step;

            while (delayer.measure() < average_step + ahead_shift * ahead_fact) {}
            delayer.reset();

            ++frame_elapsed;
        }

    private:
        size_t counter = 0;
        const size_t counter_max;
        core::timer timer;
        core::timer delayer;
        core::timer reset_timer;

        double ahead_fact   = 1.0;
        double delayed_sum  = 0.0;
        double ahead_shift  = 0.0;
        double average_step = 1 / 10000.0;
        uint64_t frame_elapsed = 0;
        double reset_step = 2.0;
    };
} // namespace core

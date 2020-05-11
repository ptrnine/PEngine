#pragma once

#include <future>
#include <thread>
#include "types.hpp"
#include "container_extensions.hpp"
#include "time.hpp"

namespace core {
    using std::future;
    using std::async;
    using std::launch;
    using std::packaged_task;
    using std::thread;
    using std::future_status;

    /**
     * Do not use with deffered tasks
     */
    struct is_ready {
        using adapter = void;

        template <typename T>
        bool operator()(const future<T>& f) const {
            return f.valid() && f.wait_for(std::chrono::seconds(0)) == future_status::ready;
        }
    };

} // namespace core


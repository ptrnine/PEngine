#pragma once

#include <future>
#include <thread>
#include "types.hpp"
#include "container_extensions.hpp"
#include "try_opt.hpp"
#include "time.hpp"
#include "fiber_pool.hpp"

namespace core {
    using std::future;
    using std::async;
    using std::launch;
    using std::packaged_task;
    using std::thread;
    using std::future_status;

    template <typename T>
    concept FutureStatus = std::convertible_to<T, std::future_status> || std::convertible_to<T, job_future_status>;

    template <typename T>
    concept FutureLike = requires (T f) {
        {f.valid()} -> std::convertible_to<bool>;
        {f.get()};
        {f.wait_for(seconds(1))} -> FutureStatus;
        {f.wait_until(time_point<seconds>())} -> FutureStatus;
    };

    inline bool is_ready__(future_status fs) {
        return fs == future_status::ready;
    }

    inline bool is_ready__(job_future_status fs) {
        return fs == job_future_status::ready;
    }

    template <typename F, typename... ArgsT>
    auto async_call(F&& function, ArgsT&&... arguments) {
        return std::async(std::launch::async, std::forward<F>(function), std::forward<ArgsT>(arguments)...);
    }

    /**
     * Returns true if the future ready to retrieve
     * Do not use with deffered tasks
     */
    struct is_ready {
        using adapter = void;

        template <FutureLike T>
        bool operator()(const T& f) const {
            return f.valid() && is_ready__(f.wait_for(std::chrono::seconds(0)));
        }
    };

    template <FutureLike T>
    using future_type_t = std::decay_t<decltype(std::declval<T>().get())>;

    template <typename T>
    struct unwrap_type_if_try_opt {
        using type = T;
    };

    template <typename T>
    struct unwrap_type_if_try_opt<try_opt<T>> {
        using type = T;
    };

    /**
     * @brief Represents a resource that loading asynchronously
     *
     * @tparam F - type of the nested future
     */
    template <FutureLike F>
    class deffered_resource {
    public:
        using value_type = typename unwrap_type_if_try_opt<future_type_t<F>>::type;

        /**
         * @brief Default constructor
         *
         * Create resource with uninitialized state
         */
        deffered_resource() = default;

        /**
         * @brief Constructor from future
         *
         * @param f - moved future
         */
        deffered_resource(F&& f) noexcept: future_(move(f)) {}

        /**
         * @brief Copy assignment from future
         *
         * @param f - moved future
         *
         * @return reference to self
         */
        deffered_resource& operator= (F&& f) noexcept { storage_.reset(); future_ = move(f); return *this; }

        /**
         * @brief Move constructor
         *
         * @param resource - deffered resource
         */
        deffered_resource(deffered_resource&& resource) noexcept = default;

        /**
         * @brief Move assignment
         *
         * @param resource - deffered resource
         *
         * @return reference to self
         */
        deffered_resource& operator= (deffered_resource&& resource) noexcept = default;

        /**
         * @brief Copy constructor
         *
         * !!! Awaits for the future in the copied resource
         *
         * @param resource - value to be copied
         */
        deffered_resource(const deffered_resource& resource) {
            resource.wait();
            storage_ = resource.get();
        }

        /**
         * @brief Copy assignment
         *
         * !!! Awaits for the future in the copied resource
         *
         * @param resource - value to be copied
         *
         * @return reference to self
         */
        deffered_resource& operator= (const deffered_resource& resource) {
            resource.wait();
            storage_ = resource.get();
        }

        ~deffered_resource() = default;

        /**
         * @brief Wait for the nested resource
         */
        void wait() const {
            try {
                if (future_ && future_->valid())
                    storage_ = move(future_->get());
            } catch (...) {
                storage_ = std::current_exception();
            }
        }

        /**
         * @brief Gets try_opt with the nested resource
         *
         * Awaits for resource if it is not ready
         *
         * @return reference to try_opt with the nested resource
         */
        [[nodiscard]]
        try_opt<value_type>& get() & {
            wait();
            return storage_;
        }

        /**
         * @brief Gets try_opt with the nested resource
         *
         * Awaits for resource if it is not ready
         *
         * @return const reference to try_opt with the nested resource
         */
        [[nodiscard]]
        const try_opt<value_type>& get() const& {
            wait();
            return storage_;
        }

        /**
         * @brief Gets the nested resource
         *
         * @throw Any exception that the future rethrows
         *
         * Awaits for resource if it is not ready
         *
         * @return reference to the nested value
         */
        [[nodiscard]]
        value_type& get_unwrap() & {
            wait();
            return storage_.value();
        }

        /**
         * @brief Gets the nested resource
         *
         * @throw Any exception that the future rethrows
         *
         * Awaits for resource if it is not ready
         *
         * @return const reference to the nested value
         */
        [[nodiscard]]
        const value_type& get_unwrap() const& {
            wait();
            return storage_.value();
        }

        /**
         * @brief Gets try_opt with the nested resource
         *
         * Awaits for resource if it is not ready
         *
         * @return rvalue reference to try_opt with the nested resource
         */
        [[nodiscard]]
        try_opt<value_type>&& get() && {
            wait();
            return move(storage_);
        }

        /**
         * @brief Gets the nested resource
         *
         * @throw Any exception that the future rethrows
         *
         * Awaits for resource if it is not ready
         *
         * @return rvalue reference to the nested value
         */
        [[nodiscard]]
        value_type&& get_unwrap() && {
            wait();
            return move(move(storage_).value());
        }

        /**
         * @brief Checks if the resource is uninitialized
         *
         * @return true if the resource is uninitialized, otherwise false
         */
        [[nodiscard]]
        bool empty() const {
            return !future_ && !storage_;
        }

        /**
         * @brief Checks if the result is available
         *
         * @return true if the result is available, otherwise false
         */
        [[nodiscard]]
        bool is_ready() const {
            return future_ && (storage_ || *future_ / core::is_ready());
        }

    private:
        mutable optional<F>         future_;
        mutable try_opt<value_type> storage_;
    };

} // namespace core


#pragma once

#include <future>
#include <thread>
#include <boost/fiber/unbuffered_channel.hpp>
#include <boost/fiber/buffered_channel.hpp>
#include <boost/fiber/channel_op_status.hpp>
#include "types.hpp"
#include "bits.hpp"
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

    using boost::fibers::channel_op_status;

    namespace details
    {
        class channel_holder_base_generic { // NOLINT
        public:
            virtual ~channel_holder_base_generic() = default;
            virtual void close() = 0;
            virtual bool is_closed() const = 0;
        };

        class channel_mem {
            SINGLETON_IMPL(channel_mem);

        public:
            channel_mem()  = default;
            ~channel_mem() = default;

            void register_chan(channel_holder_base_generic* chan) {
                _mem.insert(chan, true);
            }

            void unregister_chan(channel_holder_base_generic* chan) {
                _mem.erase(chan);
            }

            void close_all() {
                for (auto [chan, _] : _mem.lock_table())
                    chan->close();
            }

        private:
            mpmc_hash_map<channel_holder_base_generic*, bool> _mem;
        };

        inline void kek() {
            fibers::buffered_channel<int> a(4);
        }

        template <typename T>
        class channel_holder_base : public channel_holder_base_generic {
        public:
            virtual channel_op_status pop(T& value) = 0;
            virtual channel_op_status pop_wait_for(T& value, nanoseconds timeout_duration) = 0;
            virtual channel_op_status pop_wait_until(T& value, time_point<steady_clock, nanoseconds> timeout_time) = 0;
            virtual T                 value_pop() = 0;
            virtual channel_op_status push(const T& value) = 0;
            virtual channel_op_status push(T&& value) = 0;
            virtual channel_op_status push_wait_for(const T& value, nanoseconds timeout_duration) = 0;
            virtual channel_op_status push_wait_for(T&& value, nanoseconds timeout_duration) = 0;
            virtual channel_op_status push_wait_until(const T& value, time_point<steady_clock, nanoseconds> timeout_time) = 0;
            virtual channel_op_status push_wait_until(T&& value, time_point<steady_clock, nanoseconds> timeout_time) = 0;
            virtual channel_op_status try_pop(T& value) = 0;
            virtual channel_op_status try_push(const T& value) = 0;
            virtual channel_op_status try_push(T&& value) = 0;
        };

        template <template <typename> class ChanT, typename T>
        class channel_holder : public channel_holder_base<T> { // NOLINT
        public:
            using value_type = T;

            template <typename = void>
            requires std::same_as<ChanT<T>, boost::fibers::buffered_channel<T>>
            channel_holder(size_t capacity = 0): _mpmc(next_power_of_two(capacity)) {
                channel_mem::instance().register_chan(this);
            }

            template <typename = void>
            requires std::same_as<ChanT<T>, boost::fibers::unbuffered_channel<T>>
            channel_holder([[maybe_unused]] size_t = 0) {
                channel_mem::instance().register_chan(this);
            }

            ~channel_holder() override {
                channel_mem::instance().unregister_chan(this);
            }

            /*
            friend auto begin(channel& chan) {
                return boost::fibers::begin(*chan);
            }

            friend auto end(channel& chan) {
                return boost::fibers::end(*chan);
            }
            */

            void close() final {
                _mpmc.close();
            }

            [[nodiscard]]
            bool is_closed() const final {
                return _mpmc.is_closed();
            }

            T value_pop() final {
                return _mpmc.value_pop();
            }

            channel_op_status pop(T& value) final {
                return _mpmc.pop(value);
            }

            channel_op_status pop_wait_for(T& value, nanoseconds timeout_duration) final {
                return _mpmc.pop_wait_for(value, timeout_duration);
            }

            channel_op_status pop_wait_until(T& value, time_point<steady_clock, nanoseconds> timeout_time) final {
                return _mpmc.pop_wait_until(value, timeout_time);
            }

            channel_op_status push(const T& value) final {
                return _mpmc.push(value);
            }

            channel_op_status push(T&& value) final {
                return _mpmc.push(move(value));
            }

            channel_op_status push_wait_for(const T& value, nanoseconds timeout_duration) final {
                return _mpmc.push_wait_for(value, timeout_duration);
            }

            channel_op_status push_wait_for(T&& value, nanoseconds timeout_duration) final {
                return _mpmc.push_wait_for(move(value), timeout_duration);
            }

            channel_op_status push_wait_until(const T& value, time_point<steady_clock, nanoseconds> timeout_time) final {
                return _mpmc.push_wait_until(value, timeout_time);
            }

            channel_op_status push_wait_until(T&& value, time_point<steady_clock, nanoseconds> timeout_time) final {
                return _mpmc.push_wait_until(move(value), timeout_time);
            }

            channel_op_status try_pop(T& result) final {
                if constexpr (std::same_as<ChanT<T>, boost::fibers::buffered_channel<T>>)
                    return _mpmc.try_pop(result);
                else {
                    auto status = _mpmc.pop_wait_for(result, 0s);
                    if (status == channel_op_status::timeout)
                        status = channel_op_status::empty;
                    return status;
                }
            }

            channel_op_status try_push(const T& value) final {
                if constexpr (std::same_as<ChanT<T>, boost::fibers::buffered_channel<T>>)
                    return _mpmc.try_push(value);
                else {
                    auto status = _mpmc.push_wait_for(value, 0s);
                    if (status == channel_op_status::timeout)
                        status = channel_op_status::empty;
                    return status;
                }
            }

            channel_op_status try_push(T&& value) final {
                if constexpr (std::same_as<ChanT<T>, boost::fibers::buffered_channel<T>>)
                    return _mpmc.try_push(move(value));
                else {
                    auto status = _mpmc.push_wait_for(move(value), 0s);
                    if (status == channel_op_status::timeout)
                        status = channel_op_status::empty;
                    return status;
                }
            }

        private:
            ChanT<T> _mpmc;
        };
    } // namespace details

    template <typename T>
    class channel {
    public:
        using value_type = T;

        channel(size_t size = 0) {
            if (size == 0)
                _holder = make_shared<details::channel_holder<boost::fibers::unbuffered_channel, T>>();
            else
                _holder = make_shared<details::channel_holder<boost::fibers::buffered_channel, T>>(size);
        }

        friend T& operator<<(T& value, channel& chan) {
            return value = chan._holder->value_pop();
        }

        channel_op_status pop(T& value) {
            return _holder->pop(value);
        }

        template <typename Rep, typename Period>
        channel_op_status pop_wait_for(T& value, duration<Rep, Period> timeout_duration) {
            return _holder->pop_wait_for(value, duration_cast<nanoseconds>(timeout_duration));
        }

        template <typename DurationT>
        channel_op_status pop_wait_until(T& value, time_point<steady_clock, DurationT> timeout_point) {
            return _holder->pop_wait_until(
                value, time_point_cast<time_point<steady_clock, nanoseconds>>(timeout_point));
        }

        T value_pop() {
            return _holder->value_pop();
        }

        channel_op_status push(const T& value) {
            return _holder->push(value);
        }

        channel_op_status push(T&& value) {
            return _holder->push(move(value));
        }

        template <typename Rep, typename Period>
        channel_op_status push_wait_for(const T& value, duration<Rep, Period> timeout_duration) {
            return _holder->push_wait_for(value, duration_cast<nanoseconds>(timeout_duration));
        }

        template <typename Rep, typename Period>
        channel_op_status push_wait_for(T&& value, duration<Rep, Period> timeout_duration) {
            return _holder->push_wait_for(move(value), duration_cast<nanoseconds>(timeout_duration));
        }

        template <typename DurationT>
        channel_op_status push_wait_until(const T& value, time_point<steady_clock, DurationT> timeout_point) {
            return _holder->push_wait_until(
                    value, time_point_cast<time_point<steady_clock, nanoseconds>>(timeout_point));
        }

        template <typename DurationT>
        channel_op_status push_wait_until(T&& value, time_point<steady_clock, DurationT> timeout_point) {
            return _holder->push_wait_until(
                    move(value), time_point_cast<time_point<steady_clock, nanoseconds>>(timeout_point));
        }

        channel_op_status try_push(const T& value) {
            return _holder->try_push(value);
        }

        channel_op_status try_push(T&& value) {
            return _holder->try_push(move(value));
        }

        channel& operator<<(const T& value) {
            auto status = _holder->push(value);
            if (status == channel_op_status::success)
                return *this;
            else
                throw std::runtime_error("Channel was closed");
        }

        channel& operator<<(T&& value) {
            auto status = _holder->push(move(value));
            if (status == channel_op_status::success)
                return *this;
            else
                throw std::runtime_error("Channel was closed");
        }

        channel_op_status try_pop(T& result) {
            return _holder->try_pop(result);
        }

        void close() {
            _holder->close();
        }

        bool is_closed() const {
            return _holder->is_closed();
        }


    private:
        shared_ptr<details::channel_holder_base<T>> _holder;
    };

    namespace details
    {
        template <typename T>
        struct is_channel : std::false_type {};

        template <typename T>
        struct is_channel<channel<T>> : std::true_type {};
    } // namespace details

    template <typename T>
    concept Channel = details::is_channel<T>::value;

    namespace details {
        template <size_t I, typename T>
        struct first_commer_element {
            T value;
        };

        template <size_t, typename T, typename V, Channel... Ts>
        void setup_variant(variant<typename Ts::value_type...>& v, T&& value) {
            v = move(value);
        }

        template <size_t I, typename T, typename VariantT, Channel...>
        void setup_variant(VariantT& v, T&& value) {
            v = first_commer_element<I, T>{move(value)};
        }


        template <typename VariantT, size_t I = 0, Channel... Ts>
        void first_commer_iter(array<channel_op_status, sizeof...(Ts)>& statuses,
                               tuple<Ts...>&                            chans,
                               VariantT&                                result,
                               tuple<typename Ts::value_type...>&       tmp) {
            if constexpr (I < sizeof...(Ts)) {
                if ((get<I>(statuses) = get<I>(chans).try_pop(get<I>(tmp))) == channel_op_status::success) {
                    setup_variant<I, std::tuple_element_t<I, tuple<typename Ts::value_type...>>, VariantT, Ts...>(
                            result, move(get<I>(tmp)));
                    return;
                } else {
                    return first_commer_iter<VariantT, I + 1, Ts...>(statuses, chans, result, tmp);
                }
            }
        }

        template <typename VariantT, Channel... Ts>
        VariantT first_commer(tuple<Ts...> chans) {
            VariantT result;
            tuple<typename Ts::value_type...> tmp_storage;
            bool active = true;

            while (active) {
                array<channel_op_status, sizeof...(Ts)> statuses;
                /* Use timeout for unset values */
                std::fill(statuses.begin(), statuses.end(), channel_op_status::timeout);
                first_commer_iter<VariantT>(statuses, chans, result, tmp_storage);

                size_t success_count = 0;
                size_t closed_count = 0;

                for (auto s : statuses) {
                    if (s == channel_op_status::success)
                        ++success_count;
                    else if (s == channel_op_status::closed)
                        ++closed_count;
                }

                if (success_count > 0 || closed_count == sizeof...(Ts))
                    active = false;

                this_job::yield();
            }

            return result;
        }

        template <size_t... Idxs, Channel... Ts>
        auto first_commer(std::index_sequence<Idxs...>, Ts... channels) {
            return first_commer<variant<first_commer_element<Idxs, typename Ts::value_type>...>>(
                tuple{move(channels)...});
        }
    }

    template <Channel T1, Channel T2, Channel... Ts>
    variant<typename T1::value_type, typename T2::value_type, typename Ts::value_type...>
    first_commer_variant(T1 chan1, T2 chan2, Ts... channels) {
        return details::first_commer<
            variant<typename T1::value_type, typename T2::value_type, typename Ts::value_type...>>(
            tuple{move(chan1), move(chan2), move(channels)...});
    }

    template <Channel T1, Channel T2, Channel... Ts>
    auto first_commer(T1 chan1, T2 chan2, Ts... channels) {
        return details::first_commer(
            std::make_index_sequence<sizeof...(Ts) + 2>(), move(chan1), move(chan2), move(channels)...);
    }

    class async_timer {
    public:
        /* TODO: replace with core::channel */
        using buff_chan_ptr = shared_ptr<fibers::buffered_channel<function<void()>>>;

        static constexpr size_t MAX_CALLBACKS = 16;

        async_timer(const async_timer&) = delete;
        async_timer& operator=(const async_timer&) = delete;
        async_timer(async_timer&&) noexcept = default;
        async_timer& operator=(async_timer&&) noexcept = default;
        ~async_timer() noexcept = default;

        template <typename Rep, typename Period>
        async_timer(duration<Rep, Period> iduration): _end(steady_clock::now() + iduration) {}

        template <typename Rep, typename Period>
        async_timer(duration<Rep, Period> iduration, const function<void()>& callback):
            async_timer(iduration)
        {
            _function_chan->push(callback);
            notifier();
        }

        [[nodiscard]]
        bool expired() const {
            return steady_clock::now() >= _end; // NOLINT
        }

        time_point<steady_clock, nanoseconds> // NOLINT
        wait() const {
            while (steady_clock::now() < _end) // NOLINT
                this_job::yield();
            return steady_clock::now();
        }

        channel<time_point<steady_clock, nanoseconds>> notifier() {
            if (!_chan) {
                if (!_function_chan)
                    _function_chan =
                        make_shared<fibers::buffered_channel<function<void()>>>(MAX_CALLBACKS);

                /* Non-blocking output */
                _chan = channel<time_point<steady_clock, nanoseconds>>(2);

                submit_job([c = *_chan, f = _function_chan, e = _end]() mutable {
                    this_job::sleep_until(e);
                    c << steady_clock::now();

                    f->close();
                    for (const auto& func : *f) func();
                });
            }
            return *_chan;
        }

        void postpone(const function<void()>& callback) {
            if (!_function_chan)
                _function_chan =
                    make_shared<fibers::buffered_channel<function<void()>>>(MAX_CALLBACKS);

            switch (_function_chan->try_push(callback)) {
            case channel_op_status::full: throw std::runtime_error("Limit for postponed tasks was reached");
            case channel_op_status::closed: throw std::runtime_error("Timer expired");
            default: break;
            }
            notifier();
        }

        channel_op_status try_postpone(const function<void()>& callback) {
            if (!_function_chan)
                _function_chan =
                    make_shared<fibers::buffered_channel<function<void()>>>(MAX_CALLBACKS);

            return _function_chan->try_push(callback);
        }

    private:
        /* TODO: replace with core::channel */
        buff_chan_ptr                                            _function_chan;
        optional<channel<time_point<steady_clock, nanoseconds>>> _chan;
        time_point<steady_clock, nanoseconds>                    _end;
    };

} // namespace core


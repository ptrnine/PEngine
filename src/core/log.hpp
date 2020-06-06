#pragma once

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

#include "helper_macros.hpp"
#include "print.hpp"
#include "types.hpp"

namespace core
{

/**
 * @brief Represetns a generic stream
 */
class log_output_stream {
public:
    log_output_stream(): _holder(make_unique<holder<std::ofstream>>(nullptr)) {}
    log_output_stream(std::ofstream ofs): _holder(make_unique<holder<std::ofstream>>(move(ofs))) {}
    log_output_stream(std::ostream& os): _holder(make_unique<holder<std::ostream&>>(os)) {}
    log_output_stream(std::stringstream ss): _holder(make_unique<holder<std::stringstream>>(move(ss))) {}

    /**
     * @brief Creates the log stream with with stdout
     *
     * @return the log stream
     */
    static log_output_stream std_out() {
        return log_output_stream(std::cout);
    }

    /**
     * @brief Writes a string to the stream
     *
     * @param data - the string to be written
     */
    void write(string_view data) {
        _holder->write(data);
    }

    /**
     * @brief Flush the stream
     */
    void flush() {
        _holder->flush();
    }

    /**
     * @brief Casts the stream to the string if it possible
     *
     * @return the optional with the stream data or nullopt
     */
    [[nodiscard]]
    optional<string> to_string() const {
        return _holder->to_string();
    }

private:
    struct holder_base {
        holder_base()          = default;
        virtual ~holder_base() = default;

        holder_base(const holder_base&) = delete;
        holder_base(holder_base&&)      = delete;

        holder_base& operator=(const holder_base&) = delete;
        holder_base& operator=(holder_base&&) = delete;

        virtual void write(string_view data) = 0;
        virtual void flush()                 = 0;

        [[nodiscard]] virtual optional<string> to_string() const = 0;
    };

    template <typename T>
    struct holder : holder_base {
        template <typename U>
        holder(U&& obj): object(forward<U>(obj)) {}

        void write(string_view data) override {
            object.write(data.data(), static_cast<std::streamsize>(data.size()));
        }

        void flush() override {
            object.flush();
        }

        [[nodiscard]] optional<string> to_string() const override {
            if constexpr (std::is_same_v<T, std::stringstream>)
                return object.str();
            else
                return nullopt;
        }

        T object;
    };

private:
    unique_ptr<holder_base> _holder;
};


/**
 * @brief Represetns a logger
 *
 * All member functions is thread-safe
 */
class logger {
    SINGLETON_IMPL(logger);

public:
    enum Type { Details = 0, Message = 1, Warning, Error };

    static constexpr array<string_view, 4> str_types = {": ", ": ", " WARNING: ", " ERROR: "};

    /**
     * @brief Constructs logger with std::cout stream
     */
    logger() {
        add_output_stream("stantard output", std::cout);
    }

    ~logger() = default;


    /**
     * @brief Flush all logger streams
     */
    void flush() {
        std::lock_guard lock(mtx);
        for (auto& val : _streams) val.second.flush();
    }

    /**
     * @brief Writes a data to all logger streams
     *
     * @param type - the type of message
     * @param data - the data to be written
     */
    void write(Type type, string_view data) {
        std::lock_guard lock(mtx);
        for (auto& val : _streams) {
            auto message = string(str_types.at(type)) + string(data);

            val.second.write(message);
            val.second.write("\n");
            val.second.flush();
        }
    }

    /**
     * @brief Formats and writes data to all logger streams
     *
     * @tparam Ts - types of values
     * @param type - the type of message
     * @param fmt - the format string
     * @param args - arguments for formatting
     */
    template <typename... Ts>
    void write(Type type, string_view fmt, Ts&&... args) {
        write(type, format(fmt, forward<Ts>(args)...));
    }

    /**
     * @brief Formats and writes data to all logger streams using Message type
     *
     * @tparam Ts - types of values
     * @param fmt - the format string
     * @param args - arguments for formatting
     */
    template <typename... Ts>
    void write(string_view fmt, Ts&&... args) {
        write(Message, fmt, forward<Ts>(args)...);
    }

    /**
     * @brief Add or replace the output stream by the key
     *
     * @tparam Ts - types of arguments to log_output_stream constructor
     * @param key - the key
     * @param args - arguments to log_output_stream constructor
     */
    template <typename... Ts>
    void add_output_stream(string_view key, Ts&&... args) {
        std::lock_guard lock(mtx);
        _streams.insert_or_assign(string(key), log_output_stream(forward<Ts>(args)...));
    }

    /**
     * @brief Removes output stream by the key
     *
     * @param key - the key
     */
    void remove_output_stream(string_view key) {
        std::lock_guard lock(mtx);
        _streams.erase(string(key));
    }

    hash_map<string, log_output_stream> _streams;
    std::mutex                          mtx;
};

inline logger& log() {
    return logger::instance();
}
} // namespace core

#define LOG(...) core::logger::instance().write(__VA_ARGS__)

#define LOG_WARNING(...) core::logger::instance().write(core::logger::Warning, __VA_ARGS__)

#define LOG_ERROR(...) core::logger::instance().write(core::logger::Error, __VA_ARGS__)

#ifndef NDEBUG
    #define DLOG(...) core::logger::instance().write(__VA_ARGS__)

    #define DLOG_WARNING(...) core::logger::instance().write(core::logger::Warning, __VA_ARGS__)

    #define DLOG_ERROR(...) core::logger::instance().write(core::logger::Error, __VA_ARGS__)
#else
    #define DLOG(...)         void(0)
    #define DLOG_WARNING(...) void(0)
    #define DLOG_ERROR(...)   void(0)
#endif

#ifdef ENABLE_DETAILED_LOG
    #define LOG_DETAILS(...) core::logger::instance().write(core::logger::Details, __VA_ARGS__)
#else
    #define LOG_DETAILS(...) void(0)
#endif

#ifdef ENABLE_CALL_TRACE
    #define CALL_TRACE(...)                                                                                            \
        core::logger::instance().write(core::logger::Details, "{}", #__VA_ARGS__);                                     \
        __VA_ARGS__
#else
    #define CALL_TRACE(...) __VA_ARGS__
#endif


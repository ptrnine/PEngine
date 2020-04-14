#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>

#include "helper_macros.hpp"
#include "types.hpp"
#include "print.hpp"

namespace core
{

    class log_output_stream {
    public:
        log_output_stream()                    : _holder(make_unique<holder < std::ofstream>>(nullptr)) {}
        log_output_stream(std::ofstream ofs)   : _holder(make_unique<holder < std::ofstream>>(move(ofs))) {}
        log_output_stream(std::ostream& os)    : _holder(make_unique<holder < std::ostream &>>(os)) {}
        log_output_stream(std::stringstream ss): _holder(make_unique<holder < std::stringstream>>(move(ss))) {}

        static log_output_stream StdOut() {
            return log_output_stream(std::cout);
        }

        void write(string_view data) const {
            _holder->write(data);
        }

        void flush() const {
            _holder->flush();
        }

        optional<string> to_string() const {
            return _holder->to_string();
        }

    private:
        struct holder_base {
            virtual ~holder_base() {}
            virtual void write(string_view data) = 0;
            virtual void flush() = 0;
            virtual optional<string> to_string() const = 0;
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

            optional<string> to_string() const override {
                if constexpr (std::is_same_v<T, std::stringstream>) {
                    return object.str();
                } else {
                    return nullopt;
                }
            }

            T object;
        };

    private:
        unique_ptr<holder_base> _holder;
    };


    class logger {
        SINGLETON_IMPL(logger);

    public:
        enum Type {
            Message = 0, Warning, Error
        };

        static constexpr const char* str_types[] = {
            ": ", " WARNING: ", " ERROR: "
        };

        logger() {
            _streams.reserve(256);
            add_output_stream("stantard output", std::cout);
        }

        void flush() {
            std::lock_guard lock(mtx);
            for (auto& val : _streams)
                val.second.flush();
        }

        void write(Type type, string_view data) {
            std::lock_guard lock(mtx);
            for (auto& val : _streams) {
                string message = str_types[type];
                message += data;

                val.second.write(message);
                val.second.write("\n");
                val.second.flush();
            }
        }

        template <typename... Ts>
        void write(Type type, string_view fmt, Ts&&... args) {
            write(type, format(fmt, forward<Ts>(args)...));
        }

        template <typename... Ts>
        void write(string_view fmt, Ts&&... args) {
            write(Message, fmt, forward<Ts>(args)...);
        }

        template <typename... Ts>
        void add_output_stream(string_view key, Ts&&... args) {
            std::lock_guard lock(mtx);
            _streams.insert_or_assign(string(key), log_output_stream(forward<Ts>(args)...));
        }

        void remove_output_stream(string_view key) {
            std::lock_guard lock(mtx);
            _streams.erase(string(key));
        }

        hash_map<string, log_output_stream> _streams;
        std::mutex mtx;
    };

    inline logger& log() {
        return logger::instance();
    }
} // namespace core

#define LOG(...) \
core::logger::instance().write(__VA_ARGS__)

#define LOG_WARNING(...) \
core::logger::instance().write(core::logger::Warning, __VA_ARGS__)

#define LOG_ERROR(...) \
core::logger::instance().write(core::logger::Error, __VA_ARGS__)

#ifdef DE_DEBUG
    #define DLOG(...) \
    core::logger::instance().write(__VA_ARGS__)

    #define DLOG_WARNING(...) \
    core::logger::instance().write(core::logger::Warning, __VA_ARGS__)

    #define DLOG_ERROR(...) \
    core::logger::instance().write(core::logger::Error, __VA_ARGS__)
#else
    #define DLOG        (...) void(0)
    #define DLOG_WARNING(...) void(0)
    #define DLOG_ERROR  (...) void(0)
#endif


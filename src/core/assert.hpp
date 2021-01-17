#pragma once

#include "log.hpp"
#include "print.hpp"

#define PeRelRequire(EXPR)                                                                         \
    core_details::require_impl(static_cast<bool>(EXPR), __FILE__, __FUNCTION__, __LINE__, #EXPR)
#define PeRelRequireF(EXPR, FMT, ...)                                                              \
    core_details::require_impl(                                                                    \
        static_cast<bool>(EXPR), __FILE__, __FUNCTION__, __LINE__, #EXPR, FMT, __VA_ARGS__)

#define PeRelAssert(EXPR)                                                                          \
    core_details::assert_impl(static_cast<bool>(EXPR), __FILE__, __FUNCTION__, __LINE__, #EXPR)
#define PeRelAssertF(EXPR, FMT, ...)                                                               \
    core_details::assert_impl(                                                                     \
        static_cast<bool>(EXPR), __FILE__, __FUNCTION__, __LINE__, #EXPR, FMT, __VA_ARGS__)


#define PeRelAbort()          PeRelRequire(false)
#define PeRelAbortF(FMT, ...) PeRelRequireF(false, FMT, __VA_ARGS__)

#ifdef DEBUG
    #define PeRequire(EXPR)            PeRelRequire(EXPR)
    #define PeRequireF(EXPR, FMT, ...) PeRelRequireF(EXPR, FMT, __VA_ARGS__)
    #define PeAbort()                  PeRequire(false)
    #define PeAbortF(FMT, ...)         PeRequireF(false, FMT, __VA_ARGS__)

    #define PeAssert(EXPR)            PeRelAssert(EXPR)
    #define PeAssertF(EXPR, FMT, ...) PeRelAssertF(EXPR, FMT, __VA_ARGS__)
    #define PeTerminate()             PeAssert(false)
    #define PeTerminateF(FMT, ...)    PeAssertF(false, FMT, __VA_ARGS__)
#else
    #define PeRequire(EXPR)            void(0)
    #define PeRequireF(EXPR, FMT, ...) void(0)
    #define PeAbort()                  void(0)
    #define PeAbortF(FMT, ...)         void(0)

    #define PeAssert(EXPR)            void(0)
    #define PeAssertF(EXPR, FMT, ...) void(0)
    #define PeTerminate()             void(0)
    #define PeTerminateF(FMT, ...)    void(0)
#endif

namespace core_details
{
inline void require_impl(
    bool expr, core::string_view file, core::string_view func, int line, core::string_view strexp) {
    if (!expr) {
        auto msg = core::format("\nFATAL ERROR:\nfile: {}:{}\nfunc: {}\nline: {}\nexpr: {}\n",
                                file,
                                line,
                                func,
                                line,
                                strexp);
        pe_throw std::runtime_error(msg);
    }
}

template <typename... ArgT>
inline void require_impl(bool              expr,
                         core::string_view file,
                         core::string_view func,
                         int               line,
                         core::string_view strexp,
                         core::string_view fmt,
                         ArgT&&... args) {
    if (!expr) {
        auto msg =
            core::format("\nFATAL ERROR:\nfile: {}:{}\nfunc: {}\nline: {}\nexpr: {}\nwhat: {}\n",
                         file,
                         line,
                         func,
                         line,
                         strexp,
                         ::core::format(fmt, core::forward<ArgT>(args)...));
        pe_throw std::runtime_error(msg);
    }
}

inline void assert_impl(
    bool expr, core::string_view file, core::string_view func, int line, core::string_view strexp) {
    if (!expr) {
        std::stringstream ss;
        ss << boost::stacktrace::stacktrace(STACKTRACE_SKIP, STACKTRACE_MAX_DEPTH);

        LOG_ERROR("\nfile: {}:{}\nfunc: {}\nline: {}\nexpr: {}\n\n\n*** STACK TRACE ***\n\n{}",
                  file,
                  line,
                  func,
                  line,
                  strexp,
                  ss.str());
        std::terminate();
    }
}

template <typename... ArgT>
inline void assert_impl(bool              expr,
                        core::string_view file,
                        core::string_view func,
                        int               line,
                        core::string_view strexp,
                        core::string_view fmt,
                        ArgT&&... args) {
    if (!expr) {
        std::stringstream ss;
        ss << boost::stacktrace::stacktrace(STACKTRACE_SKIP, STACKTRACE_MAX_DEPTH);

        LOG_ERROR("\nfile: {}:{}\nfunc: {}\nline: {}\nexpr: {}\nwhat: {}\n\n\n*** STACK TRACE ***\n\n{}",
                  file,
                  line,
                  func,
                  line,
                  strexp,
                  ::core::format(fmt, core::forward<ArgT>(args)...),
                  ss.str());
        std::terminate();
    }
}
} // namespace core_details

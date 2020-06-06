#pragma once

#include "log.hpp"
#include "print.hpp"

#define RASSERT(EXPR)            assert_details::assert_impl((EXPR), __FILE__, __FUNCTION__, __LINE__, #EXPR)
#define RASSERTF(EXPR, FMT, ...) assert_details::assert_impl((EXPR), __FILE__, __FUNCTION__, __LINE__, #EXPR, FMT, __VA_ARGS__)

#define RABORT()          RASSERT(false)
#define RABORTF(FMT, ...) RASSERTF(false, FMT, __VA_ARGS__)

#ifdef DEBUG
    #define ASSERT(EXPR)            RASSERT(EXPR)
    #define ASSERTF(EXPR, FMT, ...) RASSERTF(EXPR, FMT, __VA_ARGS__)
    #define ABORT()                 ASSERT(false)
    #define ABORTF(FMT, ...)        ASSERTF(false, FMT, __VA_ARGS__)
#else
    #define ASSERT(EXPR)            void(0)
    #define ASSERTF(EXPR, FMT, ...) void(0)
    #define ABORT()                 void(0)
    #define ABORTF(FMT, ...)        void(0)
#endif

namespace assert_details
{
inline void assert_impl(bool expr, core::string_view file, core::string_view func, int line, core::string_view strexp) {
    if (!expr) {
        LOG_ERROR("\nFATAL ERROR:\nfile: {}:{}\nfunc: {}\nline: {}\nexpr: {}\n", file, line, func, line, strexp);
        core::log().flush();
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
        LOG_ERROR("\nFATAL ERROR:\nfile: {}:{}\nfunc: {}\nline: {}\nexpr: {}\nwhat: {}\n",
                  file,
                  line,
                  func,
                  line,
                  strexp,
                  ::core::format(fmt, core::forward<ArgT>(args)...));
        core::log().flush();
        std::terminate();
    }
}
} // namespace assert_details

#pragma once

#include "log.hpp"
#include "print.hpp"

#define PeRelRequire(EXPR)            assert_details::assert_impl((EXPR), __FILE__, __FUNCTION__, __LINE__, #EXPR)
#define PeRelRequireF(EXPR, FMT, ...) assert_details::assert_impl((EXPR), __FILE__, __FUNCTION__, __LINE__, #EXPR, FMT, __VA_ARGS__)

#define PeRelAbort()          PeRelRequire(false)
#define PeRelAbortF(FMT, ...) PeRelRequireF(false, FMT, __VA_ARGS__)

#ifdef DEBUG
    #define PeRequire(EXPR)            PeRelRequire(EXPR)
    #define PeRequireF(EXPR, FMT, ...) PeRelRequireF(EXPR, FMT, __VA_ARGS__)
    #define PeAbort()                  PeRequire(false)
    #define PeAbortF(FMT, ...)         PeRequireF(false, FMT, __VA_ARGS__)
#else
    #define PeRequire(EXPR)            void(0)
    #define PeRequireF(EXPR, FMT, ...) void(0)
    #define PeAbort()                  void(0)
    #define PeAbortF(FMT, ...)         void(0)
#endif

namespace assert_details
{
inline void assert_impl(bool expr, core::string_view file, core::string_view func, int line, core::string_view strexp) {
    if (!expr) {
        auto msg = core::format("\nFATAL ERROR:\nfile: {}:{}\nfunc: {}\nline: {}\nexpr: {}\n",
                file, line, func, line, strexp);
        //core::log().flush();
        throw std::runtime_error(msg);
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
        auto msg = core::format("\nFATAL ERROR:\nfile: {}:{}\nfunc: {}\nline: {}\nexpr: {}\nwhat: {}\n",
                  file,
                  line,
                  func,
                  line,
                  strexp,
                  ::core::format(fmt, core::forward<ArgT>(args)...));
        //core::log().flush();
        throw std::runtime_error(msg);
        //std::terminate();
    }
}
} // namespace assert_details

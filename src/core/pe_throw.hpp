#pragma once

#include <boost/stacktrace.hpp>
#include <boost/exception/all.hpp>

constexpr size_t STACKTRACE_MAX_DEPTH = 512;
constexpr size_t STACKTRACE_SKIP = 1;

using boost_traced_t = boost::error_info<struct tag_stacktrace, boost::stacktrace::stacktrace>;

#define pe_throw throw core_details::stacktrace_exception_helper{} /

namespace core_details
{
struct stacktrace_exception_helper {
    template <typename T>
    auto operator/(const T& e) {
#ifdef DEBUG
        return boost::enable_error_info(e) << boost_traced_t(
                   boost::stacktrace::stacktrace(STACKTRACE_SKIP, STACKTRACE_MAX_DEPTH));
#else
        return e;
#endif
    }
};
} // namespace core_details

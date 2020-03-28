#pragma once

#include <iostream>
#include "types.hpp"

#ifdef __unix

namespace {
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <climits>
}

namespace platform_dependent
{
    using core::string;
    using core::vector;

    template <typename T> requires core::Integral<T> && core::Unsigned<T>
    inline T byte_swap(T val) {
        static_assert(sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8, "Invalid type for byte_swap");

        if constexpr (sizeof(T) == 1)
            return val;
        else if constexpr (sizeof(T) == 2)
            return (val << 8) | (val >> 8);
        else if constexpr (sizeof(T) == 4)
            return __builtin_bswap32(val);
        else if constexpr (sizeof(T) == 8)
            return __builtin_bswap64(val);
    }

    inline string get_home_dir() {
        auto bufsize = ::sysconf(_SC_GETPW_R_SIZE_MAX);
        if (bufsize == -1)
            bufsize = 0x4000;

        vector<char> buffer(static_cast<vector<char>::size_type>(bufsize));

        struct passwd  pwd;
        struct passwd* result;

        int rc = ::getpwuid_r(::getuid(), &pwd, buffer.data(), buffer.size(), &result);
        if (result == nullptr) {
            std::cerr << "getpwnam_r() failed with errno = " << rc << " (" << ::strerror(rc) << ")" << std::endl;
            return "~/";
        }

        return result->pw_dir;
    }

    inline string get_exe_dir() {
        char buffer[PATH_MAX];
        auto len = ::readlink("/proc/self/exe", buffer, PATH_MAX);

        if (len != -1) {
            string result(buffer, static_cast<string::size_type>(len));
            auto found = result.rfind('/');

            if (found != string::npos)
                result.erase(found + 1);

            return result;
        } else {
            auto tmp = errno;
            std::cerr << "readlink() failed with errno = " << tmp << " (" << ::strerror(tmp) << ")" << std::endl; 
            return {};
        }
    }

    inline string get_current_dir() {
        char buffer[PATH_MAX];

        if (::getcwd(buffer, PATH_MAX) != NULL) {
            return buffer;
        } else {
            auto tmp = errno;
            std::cerr << "getcwd() failed with errno = " << tmp << " (" << ::strerror(tmp) << ")" << std::endl; 
            return {};
        }
    }

} // namespace platform_dependent

#else
    #error "Implement me!"
#endif


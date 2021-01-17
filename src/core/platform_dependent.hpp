#pragma once

#include <iostream>
#include "helper_macros.hpp"
#include "types.hpp"
#include "scope_guard.hpp"

#ifdef __unix
/*//////////////////////////////////////////
 *
 *           Unix implementation
 *
 *//////////////////////////////////////////

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <pwd.h>
#include <climits>
#include <cpuid.h>

namespace platform_dependent
{
    using core::string;
    using core::vector;

    /* TODO: implement for Windows */
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

    /* TODO: implement for Windows */
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

    /* TODO: implement for Windows */
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

    /* TODO: implement for Windows */
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

    inline void cpuid(uint info[4], uint info_type) {
        __cpuid_count(info_type, 0, info[0], info[1], info[2], info[3]);
    }

    /* TODO: implement for Windows */
    inline core::optional<core::file_stat>
    get_file_stat(const core::string& path) {
        int fd = open(path.data(), O_RDONLY | O_NONBLOCK);
        auto scope_exit = core::scope_guard{[fd](){ close(fd); }};

        /* TODO: return error */
        if (fd == -1)
            return {};

        struct stat st;
        int rc = fstat(fd, &st);

        /* TODO: return error */
        if (rc == -1)
            return {};

        close(fd);
        scope_exit.dismiss();

        core::file_stat result;
        result.size = static_cast<core::u64>(st.st_size);

        if (S_ISREG(st.st_mode))
            result.type = core::file_type::regular;
        else if (S_ISDIR(st.st_mode))
            result.type = core::file_type::directory;
        else if (S_ISFIFO(st.st_mode))
            result.type = core::file_type::pipe;
        else if (S_ISSOCK(st.st_mode))
            result.type = core::file_type::socket;
        else if (S_ISLNK(st.st_mode))
            result.type = core::file_type::symlink;
        else if (S_ISCHR(st.st_mode))
            result.type = core::file_type::char_dev;
        else if (S_ISBLK(st.st_mode))
            result.type = core::file_type::block_dev;
        else {
            /* TODO: return error */
            return {};
        }

        return result;
    }

    inline bool recursive_create_directory(const core::string& path) {
        if (path.empty() || path == "." || path == "/")
            return true;

        auto pos = path.rfind('/');
        if (pos == path.npos)
            return true;

        auto parent = path.substr(0, pos);

        if (!recursive_create_directory(parent) && errno != EEXIST)
            return false;

        if (mkdir(path.data(), 0777) == -1 && errno != EEXIST)
            throw std::runtime_error("Can't create directory at \'" + path + "\'");

        return true;
    }

    inline bool create_pipe(const core::string& path) {
        int rc = mkfifo(path.data(), 0666);
        return rc == 0;
    }

    inline std::tm localtime(std::time_t time) {
        std::tm t;
        localtime_r(&time, &t);
        return t;
    }

    inline int afterlife(int(*main_function)(int, char**), void(*afterlife_function)(), int argc, char* argv[]) {
        pid_t pid = fork();

        if (pid == 0)
            exit(main_function(argc, argv));
        else if (pid == -1) {
            fprintf(stderr, "fork failed.\n");
            return EXIT_FAILURE;
        }

        int status;
        if (waitpid(pid, &status, 0) == -1) {
            fprintf(stderr, "waitpid failed.\n");
            return EXIT_FAILURE;
        }

        int rc = EXIT_FAILURE;
        if (WIFEXITED(status))
            rc = WEXITSTATUS(status);

        afterlife_function();

        return rc;
    }
} // namespace platform_dependent



#elif _WIN32
/*//////////////////////////////////////////
 *
 *           Windows implementation
 *
 *//////////////////////////////////////////


#include <intrin.h>


namespace platform_dependent
{
    inline void cpuid(uint info[4], uint info_type) {
        __cpuidex(info, info_type, 0);
    }

} // namespace platform_dependent


#else
    #error "Implement me!"
#endif




/*//////////////////////////////////////////
 *
 *         Universal implementation
 *
 *//////////////////////////////////////////

namespace platform_dependent
{
    struct cpu_extensions_checker {
        SINGLETON_IMPL(cpu_extensions_checker);

        cpu_extensions_checker() {
            uint info[4];

            cpuid(info, 0);
            uint n_ids = info[0];

            cpuid(info, 0x80000000);
            uint n_ex_ids = info[0];

            //  Detect Features
            if (n_ids >= 0x00000001) {
                cpuid(info,0x00000001);

                mmx  = (info[3] & (1U << 23)) != 0;
                sse  = (info[3] & (1U << 25)) != 0;
                sse2 = (info[3] & (1U << 26)) != 0;
                sse3 = (info[2] & (1U <<  0)) != 0;

                ssse3 = (info[2] & (1U <<  9)) != 0;
                sse41 = (info[2] & (1U << 19)) != 0;
                sse42 = (info[2] & (1U << 20)) != 0;
                aes   = (info[2] & (1U << 25)) != 0;

                avx  = (info[2] & (1U << 28)) != 0;
                fma3 = (info[2] & (1U << 12)) != 0;

                rdrand = (info[2] & (1U << 30)) != 0;
            }
            if (n_ids >= 0x00000007) {
                cpuid(info,0x00000007);

                avx2 = (info[1] & (1U << 5)) != 0;

                bmi1        = (info[1] & (1U <<  3)) != 0;
                bmi2        = (info[1] & (1U <<  8)) != 0;
                adx         = (info[1] & (1U << 19)) != 0;
                sha         = (info[1] & (1U << 29)) != 0;
                prefetchwt1 = (info[2] & (1U <<  0)) != 0;

                avx512f    = (info[1] & (1U << 16)) != 0;
                avx512cd   = (info[1] & (1U << 28)) != 0;
                avx512pf   = (info[1] & (1U << 26)) != 0;
                avx512er   = (info[1] & (1U << 27)) != 0;
                avx512vl   = (info[1] & (1U << 31)) != 0;
                avx512bw   = (info[1] & (1U << 30)) != 0;
                avx512dq   = (info[1] & (1U << 17)) != 0;
                avx512ifma = (info[1] & (1U << 21)) != 0;
                avx512vbmi = (info[2] & (1U <<  1)) != 0;
            }
            if (n_ex_ids >= 0x80000001){
                cpuid(info,0x80000001);

                x64   = (info[3] & (1U << 29)) != 0;
                abm   = (info[2] & (1U <<  5)) != 0;
                sse4a = (info[2] & (1U <<  6)) != 0;
                fma4  = (info[2] & (1U << 16)) != 0;
                xop   = (info[2] & (1U << 11)) != 0;
            }
        }

        /*
         * Misc
         */
        bool mmx         = false;
        bool x64         = false;
        bool abm         = false;
        bool rdrand      = false;
        bool bmi1        = false;
        bool bmi2        = false;
        bool adx         = false;
        bool prefetchwt1 = false;

        /*
         * SIMD 128-bit
         */
        bool sse   = false;
        bool sse2  = false;
        bool sse3  = false;
        bool ssse3 = false;
        bool sse41 = false;
        bool sse42 = false;
        bool sse4a = false;
        bool aes   = false;
        bool sha   = false;

        /*
         * SIMD 256-bit
         */
        bool avx  = false;
        bool xop  = false;
        bool fma3 = false;
        bool fma4 = false;
        bool avx2 = false;

        /*
         * SIMD 512-bit
         */
        bool avx512f    = false; // Foundation
        bool avx512cd   = false; // Conflict Detection
        bool avx512pf   = false; // Prefetch
        bool avx512er   = false; // Exponential + Reciprocal
        bool avx512vl   = false; // Vector Length Extensions
        bool avx512bw   = false; // Byte + Word
        bool avx512dq   = false; // Doubleword + Quadword
        bool avx512ifma = false; // Integer 52-bit Fused Multiply-Add
        bool avx512vbmi = false; // Vector Byte Manipulation Instructions
    };

    inline const cpu_extensions_checker& cpu_ext_check() {
        return cpu_extensions_checker::instance();
    }

} // namespace platform_dependent

namespace core
{
inline optional<file_type> get_file_type(const string& path) {
    return platform_dependent::get_file_stat(path) / opt_map(xlambda(opt, opt.type));
}

inline optional<u64> get_file_size(const string& path) {
    return platform_dependent::get_file_stat(path) / opt_map(xlambda(opt, opt.size));
}
} // namespace core

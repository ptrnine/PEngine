#include <csignal>
#include <boost/stacktrace.hpp>
#include "args_view.hpp"
#include "fiber_pool.hpp"
#include "async.hpp"
#include "config_manager.hpp"
#include "log.hpp"

#define STACKTRACE_DUMP_FILE "~stacktrace.dump"

#define PE_HELP(STR) \
const static auto _pe_help_declaration = []() { \
    help_details::optional_help::instance(STR); \
    return 0; \
}()

#define PE_DEFAULT_ARGS(...) \
const static auto _pe_default_args_declaration = []() { \
    default_args_details::default_args::instance({__VA_ARGS__}); \
    return 0; \
}()

int pe_main(core::args_view args);

namespace help_details {
struct optional_help {
    optional_help(core::string_view ihelp = ""): help(ihelp) {}

    static optional_help& instance(core::string_view ihelp = "") {
        static optional_help h{ihelp};
        return h;
    }

    core::string_view help;
};
}

namespace default_args_details {
struct default_args {
    default_args(std::initializer_list<core::string_view> iargs): args(iargs) {}

    static default_args& instance(std::initializer_list<core::string_view> iargs = {}) {
        static default_args a{iargs};
        return a;
    }

    std::vector<core::string_view> args;
};
}

namespace details {
namespace {
    void show_help() {
        std::cout <<
            "Basic options:\n"
            "  --disable-stdout-logs                - disables stdout log output\n"
            "  --disable-file-logs                  - disables file log output\n"
            "  -h, --help                           - shows this help\n";

        if (!help_details::optional_help::instance().help.empty())
            std::cout << std::endl
                      << "Options:\n"
                      << help_details::optional_help::instance().help << std::endl;
    }

    void signal_handler(int signum) {
        std::signal(signum, SIG_DFL);
        boost::stacktrace::safe_dump_to(STACKTRACE_DUMP_FILE);
        std::raise(SIGABRT);
    }

    void stacktrace_printer() {
        auto ifs_dump = std::ifstream(STACKTRACE_DUMP_FILE, std::ios_base::in);
        if (!ifs_dump.is_open())
            return;

        auto logs_dir = core::path_eval(core::cfg_read_path("logs_dir"));
        auto log_file = core::open_cleverly(
            logs_dir, "pengine", "log", std::ios_base::out | std::ios_base::app);
        core::log().add_output_stream("log", std::move(log_file));

        auto st = boost::stacktrace::stacktrace::from_dump(ifs_dump);
        LOG_ERROR("\n\n\n!!! UNKNOWN ERROR     *** STACK TRACE ***\n\n{}",
                  boost::stacktrace::to_string(st));
    }

    int _pe_main_internal(int argc, char* argv[]) {
        /* Remove last stacktrace dump file, attach signal handler for stacktrace dump */
        std::filesystem::remove(STACKTRACE_DUMP_FILE);
        std::signal(SIGSEGV, &details::signal_handler);

        /* Finalize fiber pool and (un)buffered channels */
        auto scope_exit = core::scope_guard([] {
            core::details::channel_mem::instance().close_all();
            core::global_fiber_pool().close();
        });

        auto args = core::args_view(argc, argv, default_args_details::default_args::instance().args);

        if (args.get("--disable-stdout-logs"))
            core::log().remove_output_stream("stdout");

        if (!args.get("--disable-file-logs")) {
            auto logs_dir = core::path_eval(core::cfg_read_path("logs_dir"));
            auto log_file = core::open_cleverly(logs_dir, "pengine", "log", std::ios_base::out);
            core::log().add_output_stream("log", std::move(log_file));
        }

        if (args.get("-h") || args.get("--help")) {
            details::show_help();
            return 0;
        }

        try {
            return pe_main(args);
        }
        catch (const std::exception& e) {
            auto trace = core_details::try_get_stacktrace_str(e);
            LOG_ERROR("{}{}", e.what(), trace ? *trace : "");
            std::terminate();
        }
    }

} // namespace
} // namespace details

int main(int argc, char* argv[]) {
    //details::_pe_main_internal(argc, argv);
    return platform_dependent::afterlife(
        details::_pe_main_internal, details::stacktrace_printer, argc, argv);
}

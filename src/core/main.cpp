#include "args_view.hpp"
#include "fiber_pool.hpp"
#include "async.hpp"
#include "config_manager.hpp"
#include "log.hpp"

int pe_main(core::args_view args);


int main(int argc, char* argv[]) {
    auto args = core::args_view(argc, argv);

    if (args.get("--disable-stdout-logs"))
        core::log().remove_output_stream("stdout");

    if (!args.get("--disable-file-logs")) {
        auto logs_dir = core::path_eval(core::cfg_read_path("logs_dir"));
        auto log_file = core::open_cleverly(logs_dir, "pengine", "log");
        core::log().add_output_stream("log", std::move(log_file));
    }

    int rc = pe_main(args);

    core::details::channel_mem::instance().close_all();
    core::global_fiber_pool().close();
    return rc;
}


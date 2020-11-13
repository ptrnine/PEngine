#include "args_view.hpp"
#include "fiber_pool.hpp"
#include "async.hpp"

int pe_main(core::args_view args);

int main(int argc, char* argv[]) {
    int rc = pe_main(core::args_view(argc, argv));
    core::details::channel_mem::instance().close_all();
    core::global_fiber_pool().close();
    return rc;
}


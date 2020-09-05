#include "args_view.hpp"
#include "fiber_pool.hpp"

int pe_main(core::args_view args);

int main(int argc, char* argv[]) {
    int rc = pe_main(core::args_view(argc, argv));
    core::global_fiber_pool().close();
    return rc;
}


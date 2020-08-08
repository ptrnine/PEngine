#include "args_view.hpp"

int pe_main(core::args_view args);

int main(int argc, char* argv[]) {
    return pe_main(core::args_view(argc, argv));
}


#include "afl.cpp"
#include <core/config_manager.hpp>

using namespace core;

vector<string> afl_corpus = {
R"(
[value]
v1 = 2
v2 = 3, 4, 5
v3 = "string"

[value2]: value
v1 = 'ewrwr'
v6 = {1, 2, 3, {3, 4}}
)"
};

int afl_main(args_view, span<byte> data) {
    CFG_STATE().cfg_entry_name("afl.test.fs.cfg");
    write_file_unwrap("~afl.test.cfg", data);
    write_file_unwrap("afl.test.fs.cfg", "#include \"~afl.test.cfg\"");

    try {
        config_manager mgr;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}

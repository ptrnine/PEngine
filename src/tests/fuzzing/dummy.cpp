#include "afl.cpp"

using namespace core;

vector<string> afl_corpus = {
    "str/dsad/sad/",
    "/one/two/",
    "1",
    "///"
};

int afl_main(args_view, span<byte> data) {
    std::string str;
    str.resize(static_cast<size_t>(data.size()));

    memcpy(str.data(), data.data(), str.size());

    auto splits = str / core::split('/');

    volatile auto s = splits.size();
    s = s + 1;

    return 0;
}

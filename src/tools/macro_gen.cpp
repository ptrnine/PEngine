#include "core/match_syntax.hpp"
#include <core/main.cpp>

using namespace core;

PE_DEFAULT_ARGS("--disable-file-logs", "--disable-stdout-logs");

enum class mode {
    gen_argname
};

int pe_main(args_view args) {
    auto mode_str = args.next("Missing mode");
    auto m = mode_str / core::match {
        (_it_ == "gen_argname") --> mode::gen_argname,
        noopt --> [](auto m) { throw std::runtime_error("Invalid mode " + m); }
    };

    if (m == mode::gen_argname) {
        auto count = args.by_key_default<size_t>("-c", 128); // NOLINT
        args.require_end();

        string out;

        out += "#define _pE_NARGS_(";
        for (auto i : index_seq(count))
            out += build_string("_"sv, std::to_string(i + 1), ", "sv);
        out += "N, ...) N\n"
               "#define _pE_NARGS(...) _pE_NARGS_(__VA_ARGS__, ";

        for (auto i : index_seq(count))
            out += build_string(std::to_string(count - i), ", "sv);
        out.resize(out.size() - 2);
        out += ")\n"
               "#define _pE_ARGNAME_CAT_(a, b) a ## b\n"
               "#define _pE_ARGNAME_CAT(a, b) _pE_ARGNAME_CAT_(a, b)\n";

        vector<string> arguments;
        arguments.reserve(count);

        for (auto i : index_seq(size_t(1), count + 1)) {
            auto strnum = std::to_string(i);
            arguments.push_back("a" + strnum);
            out += build_string("#define _pE_ARGNAME_"sv, strnum, "("sv);
            for (auto& a : arguments)
                out += a + ", ";
            out.resize(out.size() - 2);
            out += ") ";

            for (auto& a : arguments)
                out += build_string("#"sv, a, ", "sv);
            out.resize(out.size() - 2);
            out += "\n";
        }

        out += "#define PE_ARGNAMES(...) _pE_ARGNAME_CAT(_pE_ARGNAME_, _pE_NARGS(__VA_ARGS__))(__VA_ARGS__)\n";

        std::cout << out << std::endl;
    }

    return 0;
}

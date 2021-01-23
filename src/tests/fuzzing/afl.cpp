#include <cstdio>
#include <iostream>
#include <filesystem>
#include <core/args_view.hpp>
#include <core/main.cpp>
#include <core/platform_dependent.hpp>
#include <core/container_extensions.hpp>

namespace fs = std::filesystem;

extern core::vector<core::string> afl_corpus;

int afl_main(core::args_view args, core::span<core::byte> data);

static core::vector<core::byte> read_all_stdin_() {
    core::vector<core::byte> result;
    int c; // NOLINT
    do {
        c = std::getc(stdin);
        result.push_back(static_cast<core::byte>(c));
    } while (c != EOF);
    return result;
}

int pe_main(core::args_view args) {
    if (auto arg = args.try_next(); arg.has_value() && *arg == "GEN_RESOURCES") {
        auto workdir = fs::path(args.program_name()).filename().string() + "_workdir";
        auto corpus_dir = fs::path(platform_dependent::get_exe_dir()) / workdir;
        fs::create_directory(corpus_dir);
        corpus_dir /= "corpus";

        fs::remove_all(corpus_dir);
        fs::create_directory(corpus_dir);

        for (auto& [corpus, idx] : core::value_index_view(afl_corpus)) {
            auto path = corpus_dir / std::to_string(idx);
            core::write_file(path.string(), corpus);
        }

        return 0;
    }
    else {
        auto data = read_all_stdin_();
        return afl_main(std::move(args), data);
    }
}


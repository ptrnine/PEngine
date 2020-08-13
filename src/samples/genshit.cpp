#include <core/main.cpp>
#include <core/container_extensions.hpp>
#include <graphics/grx_color_map.hpp>
#include <graphics/grx_shader.hpp>
#include <graphics/grx_window.hpp>

using namespace core;
using namespace grx;

auto load_image(const string& path, uint bs) {
    auto img = load_color_map_unwrap<color_rgb>(path);
    auto wrapped_size = (img.size() / bs) * bs;
    return img.get_resized(wrapped_size);
}

int pe_main(args_view args) {
    if (args.get("--help")) {
        printline("Usage:\n{} [--quad=N]/[-q N] [path/to/source/image]"
                  " [path/to/sample/image]", args.program_name());
        return 0;
    }

    auto bs    = args.by_key_default<uint>({"--quad", "-q"}, 16);
    auto path1 = args.next_unwrap("Missing path to source image");
    auto path2 = args.next_unwrap("Missing path to sample image");
    args.require_end();

    auto img1 = load_image(path1, bs);
    auto img2 = load_image(path2, bs);

    for (auto src_pos : dimensional_seq(img1.size() / bs)) {
        auto src_start = src_pos * bs;
        auto best_cnv  = numlim<int>::max();
        auto best_pos  = vec{0U, 0U};

        printfl("\r{}/{}", src_pos, img1.size() / bs);
        for (auto smp_pos : dimensional_seq(img2.size() / bs)) {
            int cnv = 0;
            auto smp_start = smp_pos * bs;
            for (auto bs_pos : dimensional_seq(vec{bs, bs})) {
                auto result = abs(img2[smp_start + bs_pos].get() -
                        img1[src_start + bs_pos].get());
                cnv += result.x() + result.y() + result.z();
            }
            if (cnv < best_cnv) {
                best_cnv = cnv;
                best_pos = smp_start;
            }
        }
        for (auto bs_pos : dimensional_seq(vec{bs, bs}))
            img1[src_start + bs_pos] = img2[best_pos + bs_pos].get();
    }

    save_color_map_unwrap(img1, path1 + ".edit.jpg");

    return 0;
}


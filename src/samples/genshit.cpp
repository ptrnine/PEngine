#include <core/main.cpp>
#include <core/container_extensions.hpp>
#include <graphics/grx_color_map.hpp>
#include <graphics/grx_shader.hpp>
#include <graphics/grx_window.hpp>

using namespace core;
using namespace grx;

auto load_image(const string& path, uint bs) {
    auto img = load_color_map_unwrap<float_color_rgb>(path);
    auto wrapped_size = (img.size() / bs) * bs;
    return img.get_resized(wrapped_size);
}

int pe_main(args_view args) {
    auto bs    = args.by_key_default<uint>({"--quad", "-q"}, 16);
    auto path1 = args.next_unwrap("Missing path to target image");
    auto path2 = args.next_unwrap("Missing path to source image");
    args.require_end();

    auto img1 = load_image(path1, bs);
    auto img2 = load_image(path2, bs);

    for (auto src_pos : dimensional_seq(img1.size() / bs)) {
        auto src_start = src_pos * bs;
        auto best_cnv  = numlim<float>::max();
        auto best_pos  = vec{0U, 0U};

        printfl("\r{}/{}", src_pos, img1.size() / bs);
        for (auto smp_pos : dimensional_seq(img2.size() / bs)) {
            float cnv      = 0.f;
            auto smp_start = smp_pos * bs;
            for (auto [x, y] : dimensional_seq(vec{bs, bs})) {
                auto result = abs(img2[smp_start.y() + y][smp_start.x() + x].get() -
                        img1[src_start.y() + y][src_start.x() + x].get());
                cnv += result.x() + result.y() + result.z();
            }
            if (cnv < best_cnv) {
                best_cnv = cnv;
                best_pos = smp_start;
            }
        }
        for (auto [x, y] : dimensional_seq(vec{bs, bs}))
            img1[src_start.y() + y][src_start.x() + x] =
                img2[best_pos.y() + x][best_pos.x() + x].get();
    }

    save_color_map_unwrap(img1, path1 + ".edit.jpg");

    return 0;
}


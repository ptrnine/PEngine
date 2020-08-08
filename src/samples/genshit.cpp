#include <core/main.cpp>
#include <core/container_extensions.hpp>
#include <graphics/grx_color_map.hpp>
#include <random>

using namespace core;
using namespace grx;

using block_storage_t = vector<vector<float_color_rgb>>;

block_storage_t wrap_to_blocks(const grx_color_map<float, 3>& img, uint quad_size) {
    block_storage_t blocks;

    for (auto y : uindex_seq(0, img.size().y(), quad_size)) {
        for (auto x : uindex_seq(0, img.size().x(), quad_size)) {
            auto& back = blocks.emplace_back(quad_size*quad_size);

            for (auto h : uindex_seq(y, y + quad_size))
                for (auto w : uindex_seq(x, x + quad_size))
                    back.at((h-y)*quad_size + (w - x)) = img[h][w].get();
        }
    }

    return blocks;
}

int pe_main(args_view args) {
    auto quad_size = args.by_key_default<uint>({"--quad", "-q"}, 16);
    auto path1     = args.next_unwrap("Missing path to target image");
    auto path2     = args.next_unwrap("Missing path to source image");

    auto img = load_color_map_unwrap<float_color_rgb>(path1);
    auto wrapped_size = img.size();
    wrapped_size.x() = (wrapped_size.x() / quad_size) * quad_size;
    wrapped_size.y() = (wrapped_size.y() / quad_size) * quad_size;

    img       = img.get_resized(wrapped_size);
    auto img2 = load_color_map_unwrap<float_color_rgb>(path2).get_resized(wrapped_size);

    auto blocks     = wrap_to_blocks(img, quad_size);
    auto blocks2    = wrap_to_blocks(img2, quad_size);
    auto new_blocks = blocks;

    auto compare = [](auto& l, auto& r) {
        float pts = 0;
        for (auto& [a, b] : zip_view(l, r)) {
            auto v = a - b;
            pts += abs(v.x()) + abs(v.y()) + abs(v.z());
        }
        return pts;
    };

    for (size_t block_idx : index_view(blocks)) {
        print("\r{}/{}", block_idx, blocks.size());
        std::cout << std::flush;

        size_t found = 0;
        float points = std::numeric_limits<float>::max();
        for (size_t i : index_view(blocks2)) {
            float v = compare(blocks[block_idx], blocks2[i]);
            if (v < points) {
                found = i;
                points = v;
            }
        }

        new_blocks[block_idx] = blocks2[found];
    }

    for (size_t i : index_view(new_blocks)) {
        size_t x_start = (i * quad_size) % img.size().x();
        size_t y_start = ((i * quad_size) / img.size().x()) * quad_size;

        for (size_t k : index_seq(quad_size*quad_size))
            img[y_start + k / quad_size][x_start + k % quad_size] = new_blocks[i].at(k);
    }

    save_color_map_unwrap(img, path1 + ".edit.jpg");

    return 0;
}

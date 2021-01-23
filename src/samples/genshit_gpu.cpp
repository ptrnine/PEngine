#include <core/main.cpp>
#include <core/container_extensions.hpp>
#include <graphics/grx_color_map.hpp>
#include <graphics/grx_ssbo.hpp>
#include <graphics/grx_shader.hpp>
#include <graphics/grx_texture.hpp>
#include <graphics/grx_window.hpp>

using namespace core;
using namespace grx;

auto load_texture(const string& path, uint bs) {
    auto img = load_color_map<float_color_rgba>(path);
    auto wrapped_size = (img.size() / bs) * bs;
    return grx_texture(img.get_resized(wrapped_size));
}

int pe_main(args_view args) {
    if (args.get("--help")) {
        printline("Usage:\n{} [--quad=N]/[-q N] [path/to/source/image]"
                  " [path/to/sample/image]", args.program_name());
        return 0;
    }

    auto fake_wnd = grx_window("fake", {100, 100});
    fake_wnd.make_current();

    auto bs    = args.by_key_default<uint>({"--quad", "-q"}, 16);
    auto path1 = args.next("Missing path to source image");
    auto path2 = args.next("Missing path to sample image");
    args.require_end();

    auto txt1  = load_texture(path1, bs);
    auto txt2  = load_texture(path2, bs);

    auto shader = grx_shader_program::create_shared(grx_shader<shader_type::compute>{
        "layout(local_size_variable) in;"
        "uniform int bs;"
        "uniform ivec2 src_size;"
        "uniform ivec2 smp_size;"
        "layout(rgba8, binding = 0) uniform restrict image2D src_img;"
        "layout(rgba8, binding = 1) uniform readonly image2D smp_img;"
        "void main() {"
        "    int   id_x      = int(gl_GlobalInvocationID.x);"
        "    ivec2 src_bs    = src_size / bs;"
        "    ivec2 smp_bs    = smp_size / bs;"
        "    ivec2 src_start = ivec2(mod(id_x, src_bs.x), id_x / src_bs.x) * bs;"
        "    ivec2 best_pos  = ivec2(0, 0);"
        "    float best_cnv  = 99999999.0;"
        "    for (int by = 0; by < smp_bs.y; ++by) {"
        "        for (int bx = 0; bx < smp_bs.x; ++bx) {"
        "            float cnv       = 0.0;"
        "            ivec2 smp_start = ivec2(bx, by) * bs;"
        "            for (int y = 0; y < bs; ++y) {"
        "                for (int x = 0; x < bs; ++x) {"
        "                    vec4 src_color = imageLoad(src_img, src_start + ivec2(x, y));"
        "                    vec4 smp_color = imageLoad(smp_img, smp_start + ivec2(x, y));"
        "                    vec4 result    = abs(smp_color - src_color);"
        "                    cnv += result.x + result.y + result.z;"
        "                }"
        "            }"
        "            if (cnv < best_cnv) {"
        "                best_cnv = cnv;"
        "                best_pos = smp_start;"
        "            }"
        "        }"
        "    }"
        "    for (int y = 0; y < bs; ++y) {"
        "        for (int x = 0; x < bs; ++x) {"
        "            vec4 color = imageLoad(smp_img, best_pos + ivec2(x, y));"
        "            imageStore(src_img, src_start + ivec2(x, y), color);"
        "        }"
        "    }"
        "}"
    });

    shader->get_uniform<int>  ("bs")       = static_cast<int>(bs);
    shader->get_uniform<vec2i>("src_size") = static_cast<vec2i>(txt1.size());
    shader->get_uniform<vec2i>("smp_size") = static_cast<vec2i>(txt2.size());

    shader->activate();
    txt1.bind_level<0>(0, texture_access::readwrite);
    txt2.bind_level<1>(0);
    shader->dispatch_compute(txt1.size().x() / bs, 1, 1, txt1.size().y() / bs, 1, 1);
    save_color_map(txt1.to_color_map(), path1 + ".edit.jpg");

    return 0;
}


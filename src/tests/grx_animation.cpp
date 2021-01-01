#include <catch2/catch.hpp>
//#include <core/fiber_pool.hpp>
#include <core/config_manager.hpp>
#include <graphics/grx_skeleton.hpp>
#include <graphics/grx_animation.hpp>

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

using namespace core;
using namespace grx;

TEST_CASE("grx_animation test") {
    /* Shutdown fibers while exit scope */
    //auto scope_exit = scope_guard([]{
    //    details::channel_mem::instance().close_all();
    //    global_fiber_pool().close();
    //});

    auto models_dir = path_eval(cfg_read_path("models_dir"));

    auto flags = aiProcess_Triangulate            |
                 aiProcess_GenBoundingBoxes       |
                 aiProcess_GenSmoothNormals       |
                 aiProcess_CalcTangentSpace       |
                 aiProcess_FlipUVs                |
                 aiProcess_JoinIdenticalVertices;

    Assimp::Importer importer;
    auto scene = importer.ReadFile(models_dir / "glock19x/glock19x.dae", flags);

    REQUIRE(scene->HasAnimations());

    auto anim           = grx_animation::from_assimp(scene->mAnimations[0]);
    auto skeleton       = grx_skeleton::from_assimp(scene);
    auto anim_optimized = anim.get_optimized(skeleton);

    serializer s;
    s.write(anim_optimized);
    auto bytes = s.data();

    auto ds = deserializer_view(bytes);

    grx_animation_optimized anim_optimized2;
    ds.read(anim_optimized2);

    auto d1 = anim_optimized.duration();
    auto d2 = anim_optimized2.duration();

    auto t1 = anim_optimized.ticks_per_second();
    auto t2 = anim_optimized2.ticks_per_second();

    REQUIRE(memcmp(&d1, &d2, sizeof(d1)) == 0);
    REQUIRE(memcmp(&t1, &t2, sizeof(t1)) == 0);
    REQUIRE(anim_optimized.channels().size() == anim_optimized2.channels().size());

    for (auto& [c1, c2] : zip_view(anim_optimized.channels(), anim_optimized2.channels())) {
        REQUIRE(c1.size() == c2.size());
        for (auto& [k1, k2] : zip_view(c1, c2)) {
            REQUIRE(memcmp(&k1.time, &k2.time, sizeof(k1.time)) == 0);
            REQUIRE(memcmp(&k1.value.position, &k2.value.position, sizeof(k1.value.position)) == 0);
            REQUIRE(memcmp(&k1.value.scaling, &k2.value.scaling, sizeof(k1.value.scaling)) == 0);
            REQUIRE(memcmp(&k1.value.rotation, &k2.value.rotation, sizeof(k1.value.rotation)) == 0);
        }
    }
}

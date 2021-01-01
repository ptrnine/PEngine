#include "graphics/grx_utils.hpp"
#include <catch2/catch.hpp>
//#include <core/fiber_pool.hpp>
#include <core/config_manager.hpp>
#include <graphics/grx_skeleton.hpp>
#include <graphics/grx_cpu_mesh_group.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

using namespace core;
using namespace grx;

TEST_CASE("grx_skeleton test") {
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

    auto skeleton           = grx_skeleton::from_assimp(scene);
    auto skeleton_optimized = skeleton.get_optimized();

    vector<string> names;
    vector<grx_aabb> aabbs;

    skeleton.traverse([&](const grx_bone_node& node) {
        names.push_back(node.name);
    });

    skeleton_optimized.traverse([&](const grx_bone_node_optimized& node) {
        aabbs.push_back(node.aabb);
    });

    REQUIRE(names.size() == aabbs.size());

    for (auto& [name, aabb] : zip_view(names, aabbs)) {
        auto laabb = skeleton.skeleton_data().aabbs[skeleton.skeleton_data().mapping.at(name)];
        REQUIRE(memcmp(&laabb, &aabb, sizeof(laabb)) == 0);
    }

    serializer s;
    s.write(skeleton_optimized);
    auto bytes = s.data();

    auto ds = deserializer_view(bytes);
    grx_skeleton_optimized skeleton_optimized2;
    ds.read(skeleton_optimized2);

    for (auto& [n1, n2] : zip_view(skeleton_optimized.storage(), skeleton_optimized2.storage())) {
        REQUIRE(memcmp(&n1.aabb, &n2.aabb, sizeof(n1.aabb)) == 0);
        REQUIRE(n1.children.size() == n2.children.size());
        if (!n1.children.empty())
            REQUIRE((&(*n1.children.begin()) - skeleton_optimized.storage().data()) ==
                    (&(*n2.children.begin()) - skeleton_optimized2.storage().data()));
        REQUIRE(n1.idx == n2.idx);
        REQUIRE(memcmp(&n1.offset, &n2.offset, sizeof(n1.offset)) == 0);
        REQUIRE(memcmp(&n1.transform, &n2.transform, sizeof(n1.offset)) == 0);
    }

    REQUIRE(skeleton_optimized.final_transforms().size() ==
            skeleton_optimized2.final_transforms().size());
    REQUIRE(memcmp(skeleton_optimized.final_transforms().data(),
                   skeleton_optimized2.final_transforms().data(),
                   sizeof(skeleton_optimized.final_transforms()[0]) *
                       skeleton_optimized.final_transforms().size()) == 0);

    REQUIRE(skeleton_optimized.depth() == skeleton_optimized2.depth());
}


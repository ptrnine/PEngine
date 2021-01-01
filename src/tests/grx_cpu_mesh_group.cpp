#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <catch2/catch.hpp>
#include <graphics/grx_cpu_mesh_group.hpp>
#include <assimp/scene.h>
#include <core/config_manager.hpp>
#include <core/serialization.hpp>
#include <core/slice_range_view.hpp>

using namespace core;
using namespace grx;

TEST_CASE("grx_cpu_mesh test") {
    /* Shutdown fibers while exit scope */
    auto scope_exit = scope_guard([]{
        core::details::channel_mem::instance().close_all();
        core::global_fiber_pool().close();
    });


    auto models_dir = path_eval(cfg_read_path("models_dir"));

    auto flags = aiProcess_Triangulate            |
                 aiProcess_GenBoundingBoxes       |
                 aiProcess_GenSmoothNormals       |
                 aiProcess_CalcTangentSpace       |
                 aiProcess_FlipUVs                |
                 aiProcess_JoinIdenticalVertices;

    std::cout << models_dir << std::endl;
    Assimp::Importer importer;
    auto scene = importer.ReadFile(models_dir / "basic/cube.dae", flags);

    using mesh_t =
    grx_cpu_mesh_group<
        mesh_buf_spec<mesh_buf_tag::index,     vector<u32>>,
        mesh_buf_spec<mesh_buf_tag::position,  vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::normal,    vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::uv,        vector<vec2f>>,
        mesh_buf_spec<mesh_buf_tag::tangent,   vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::bitangent, vector<vec3f>>>;

    auto mesh = mesh_t::from_assimp(scene);

    REQUIRE(format("{}", mesh.get<mesh_buf_tag::index>()) ==
            "{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 0, 18, 1, 3, 19, 4, "
            "6, 20, 7, 9, 21, 10, 12, 22, 13, 15, 23, 16 }");
    REQUIRE(
        format("{}", mesh.get<mesh_buf_tag::position>()) ==
        "{ { -1, 1, 1 }, { 1, -1, 1 }, { 1, 1, 1 }, { 1, -1, 1 }, { -1, -1, -1 }, { 1, -1, -1 }, { "
        "-1, -1, 1 }, { -1, 1, -1 }, { -1, -1, -1 }, { 1, 1, -1 }, { -1, -1, -1 }, { -1, 1, -1 }, "
        "{ 1, 1, 1 }, { 1, -1, -1 }, { 1, 1, -1 }, { -1, 1, 1 }, { 1, 1, -1 }, { -1, 1, -1 }, { "
        "-1, -1, 1 }, { -1, -1, 1 }, { -1, 1, 1 }, { 1, -1, -1 }, { 1, -1, 1 }, { 1, 1, 1 } }");
    REQUIRE(format("{}", mesh.get<mesh_buf_tag::normal>()) ==
            "{ { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, -1, 0 }, { 0, -1, 0 }, { 0, -"
            "1, 0 }, { -1, 0, 0 }, { -1, 0, 0 }, { -1, 0, 0 }, { 0, 0, -1 }, { 0, 0, -1 },"
            " { 0, 0, -1 }, { 1, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 1, 0 }"
            ", { 0, 1, 0 }, { 0, 0, 1 }, { 0, -1, 0 }, { -1, 0, 0 }, { 0, 0, -1 }, { 1, 0,"
            " 0 }, { 0, 1, 0 } }");
    REQUIRE(format("{}", mesh.get<mesh_buf_tag::uv>()) ==
            "{ { 0.3331336, 0.9998003 }, { 0.00019976, 0.6668664 }, { 0.000199781, 0.9998002 }, { "
            "0.6668665, 0.6668665 }, { 0.9998003, 0.9998002 }, { 0.9998003, 0.6668664 }, { "
            "0.666467, 0.333533 }, { 0.3335331, 0.6664669 }, { 0.666467, 0.6664669 }, { 0.666467, "
            "0.9998002 }, { 0.3335331, 0.6668664 }, { 0.3335331, 0.9998003 }, { 0.00019976, "
            "0.0001997948 }, { 0.3331336, 0.3331335 }, { 0.3331336, 0.0001996756 }, { 0.3331336, "
            "0.333533 }, { 0.00019976, 0.6664668 }, { 0.3331336, 0.6664669 }, { 0.3331335, "
            "0.6668664 }, { 0.6668665, 0.9998003 }, { 0.3335331, 0.3335331 }, { 0.6664669, "
            "0.6668664 }, { 0.000199781, 0.3331335 }, { 0.000199781, 0.333533 } }");
    REQUIRE(format("{}", mesh.get<mesh_buf_tag::tangent>()) ==
            "{ { -1, -1.193524e-07, 0 }, { -1, -1.193524e-07, 0 }, { -1, -1.790285e-07, 0 }, { "
            "-2.983809e-07, 0, -1 }, { -2.983809e-07, 0, -1 }, { -3.580571e-07, 0, -1 }, { 0, -1, "
            "-1.193524e-07 }, { 0, -1, -1.193524e-07 }, { 0, -1, 0 }, { 1, 1.193523e-07, 0 }, { 1, "
            "1.193523e-07, 0 }, { 1, 1.790285e-07, 0 }, { 0, -2.387047e-07, -1 }, { 0, "
            "-2.387047e-07, -1 }, { 0, -3.58057e-07, -1 }, { -1, 0, 1.193523e-07 }, { -1, 0, "
            "1.193523e-07 }, { -1, 0, 1.790285e-07 }, { -1, 0, 0 }, { -1.790286e-07, 0, -1 }, { 0, "
            "-1, -3.580571e-07 }, { 1, 0, 0 }, { 0, 0, -1 }, { -1, 0, 0 } }");
    REQUIRE(format("{}", mesh.get<mesh_buf_tag::bitangent>()) ==
            "{ { -1.790285e-07, -1, 0 }, { -1.790285e-07, -1, 0 }, { -8.951427e-08, -1, 0 }, { 1, "
            "0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1 }, { "
            "1.193524e-07, -1, 0 }, { 1.193524e-07, -1, 0 }, { 0, -1, 0 }, { 0, 1, -2.100904e-08 "
            "}, { 0, 1, -2.100904e-08 }, { 0, 1, 0 }, { 2.983808e-08, 0, 1 }, { 2.983808e-08, 0, 1 "
            "}, { 0, 0, 1 }, { -3.580571e-07, -1, 0 }, { 1, 0, 0 }, { 0, 0, 1 }, { 3.580571e-07, "
            "-1, 0 }, { 0, 1, -6.302713e-08 }, { 8.951427e-08, 0, 1 } }");

    REQUIRE(mesh == mesh.submesh(0));

    serializer s;
    s.write(mesh);

    mesh_t mesh2;
    {
        auto data = s.data();
        deserializer_view ds(data);
        ds.read(mesh2);
    }

    REQUIRE(mesh == mesh2);
    REQUIRE(mesh == mesh_t::from_assimp(scene->mMeshes[0]));

    mesh_t mesh3;
    mesh3.push_from_assimp(scene->mMeshes[0]);
    REQUIRE(mesh == mesh3);

    mesh_t mesh4;
    mesh4.push_from_assimp<mesh_buf_tag::index>(scene->mMeshes[0]);
    mesh4.set_from_assimp<mesh_buf_tag::position>(scene->mMeshes[0]);
    mesh4.set_from_assimp<mesh_buf_tag::normal>(scene->mMeshes[0]);
    mesh4.set_from_assimp<mesh_buf_tag::uv>(scene->mMeshes[0]);
    mesh4.set_from_assimp<mesh_buf_tag::tangent>(scene->mMeshes[0]);
    mesh4.set_from_assimp<mesh_buf_tag::bitangent>(scene->mMeshes[0]);

    REQUIRE(mesh == mesh4);

    mesh4.set_from_assimp<mesh_buf_tag::index>(scene->mMeshes[0], 1);
    mesh4.set_from_assimp<mesh_buf_tag::position>(scene->mMeshes[0], 1);
    mesh4.set_from_assimp<mesh_buf_tag::normal>(scene->mMeshes[0], 1);
    mesh4.set_from_assimp<mesh_buf_tag::uv>(scene->mMeshes[0], 1);
    mesh4.set_from_assimp<mesh_buf_tag::tangent>(scene->mMeshes[0], 1);
    mesh4.set_from_assimp<mesh_buf_tag::bitangent>(scene->mMeshes[0], 1);

    REQUIRE(mesh == mesh4.submesh(0));
    REQUIRE(mesh == mesh4.submesh(1));

    for (auto& [vertex, normal] : mesh4.view<mesh_buf_tag::position, mesh_buf_tag::normal>())
        vertex *= 2.f;
    REQUIRE(mesh != mesh4.submesh(0));

    auto mesh5 = load_mesh_group(models_dir + "basic/cube.dae");
    for (auto& [vertex, normal] : mesh5.view<mesh_buf_tag::position, mesh_buf_tag::normal>())
        vertex *= 2.f;

    REQUIRE(mesh4.submesh(0) == mesh5);

    save_mesh_group("./test_mesh.pemesh", mesh5);
    auto mesh6 = load_mesh_group("./test_mesh.pemesh");
    REQUIRE(mesh5 == mesh6);

    auto rc = try_async_save_mesh_group("./test_mesh.pemesh", mesh5);
    REQUIRE(rc.get());
    deffered_resource mesh7 = try_async_load_mesh_group("./test_mesh.pemesh");
    REQUIRE(mesh7.get().value() == mesh5);

    using mesh_t2 =
    grx_cpu_mesh_group<
        mesh_buf_spec<mesh_buf_tag::position,  vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::index,     vector<u32>>,
        mesh_buf_spec<mesh_buf_tag::normal,    vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::uv,        vector<vec2f>>,
        mesh_buf_spec<mesh_buf_tag::tangent,   vector<vec3f>>,
        mesh_buf_spec<mesh_buf_tag::bitangent, vector<vec3f>>>;

    auto mesh8 = try_load_mesh_group<mesh_t2>("./test_mesh.pemesh");
    REQUIRE(!mesh8);
    REQUIRE(mesh8.thrower_ptr()->error_message() == "Mesh buffers tag order differs with mesh data");
}

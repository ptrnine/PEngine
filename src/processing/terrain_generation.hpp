#include <core/container_extensions.hpp>
#include <core/vec.hpp>
#include <core/ston.hpp>
#include <graphics/grx_color_map.hpp>
#include <graphics/grx_cpu_mesh_group.hpp>

namespace prc {
using core::move;
using core::zip_view;
using core::vec3f;
using core::vec;
using core::vec2f;
using core::vec2u;
using core::u32;
using core::vector;
using core::string;
using core::build_string;
using namespace core::string_view_literals;
using std::to_string;
using grx::grx_float_color_map_r;
using grx::grx_cpu_mesh_group_t;
using grx::mesh_buf_tag;

class terrain_editor {
public:
    struct vertex_data {
        vec3f position;
        vec2f uv;
        vec3f normal           = {0.f, 0.f, 0.f};
        u32   internal_face_id = core::numlim<u32>::max();
    };

    struct face_data {
        u32 a, b, c;

        void recalc_normal(vector<vertex_data>& vertices) const {
            auto& pa = vertices[a].position;
            auto& pb = vertices[b].position;
            auto& pc = vertices[c].position;
            auto n = (pb - pa).cross(pc - pa);
            vertices[a].normal += n;
            vertices[b].normal += n;
            vertices[c].normal += n;
        }

        void set_zero_normal(std::vector<vertex_data>& vertices) const {
            vertices[a].normal = vec3f::filled_with(0.f);
            vertices[b].normal = vec3f::filled_with(0.f);
            vertices[c].normal = vec3f::filled_with(0.f);
        }
    };

    static terrain_editor generate_flat(const vec2f& size, const vec2u& split_count) {
        auto hmap = grx_float_color_map_r({2, 2});
        for (auto c : hmap)
            c = vec{0.f};
        return generate(hmap, size, 1.f, split_count);
    }

    static float terrain_gen_identity_curve(float v) {
        return v;
    }

    template <typename F = decltype(terrain_gen_identity_curve)>
    static terrain_editor generate(const grx_float_color_map_r& height_map,
                                   const vec2f&                 size,
                                   float                        max_height,
                                   const vec2u&                 split_count,
                                   F&&                          curve_func = terrain_gen_identity_curve) {
        terrain_editor res;

        res.vertex_count = split_count + vec2u{2, 2};
        res.size         = size;

        static constexpr auto take_color =
            [](const grx_float_color_map_r& height_map, const vec2f& size_f, vec2f pos, auto&& curve_func) {
                auto p  = pos * size_f;
                auto p1 = vec{std::floor(p.x()), std::floor(p.y())};
                auto p2 = vec{std::ceil(p.x()), std::ceil(p.y())};
                auto p3 = vec{p2.x(), p1.y()};
                auto p4 = vec{p1.x(), p2.y()};

                auto color1 = height_map[size_t(p1.y())][size_t(p1.x())];
                auto color2 = height_map[size_t(p2.y())][size_t(p2.x())];
                auto color3 = height_map[size_t(p3.y())][size_t(p3.x())];
                auto color4 = height_map[size_t(p4.y())][size_t(p4.x())];

                auto dst1 = (p - p1).magnitude();
                auto dst2 = (p - p2).magnitude();
                auto dst3 = (p - p3).magnitude();
                auto dst4 = (p - p4).magnitude();

                constexpr auto mindst = 0.00001f;

                if (dst1 < mindst)
                    return color1.get().x();
                if (dst2 < mindst)
                    return color2.get().x();
                if (dst3 < mindst)
                    return color3.get().x();
                if (dst4 < mindst)
                    return color4.get().x();

                dst1 = 1.f / dst1;
                dst2 = 1.f / dst2;
                dst3 = 1.f / dst3;
                dst4 = 1.f / dst4;

                auto dp1 = curve_func(dst1);
                auto dp2 = curve_func(dst2);
                auto dp3 = curve_func(dst3);
                auto dp4 = curve_func(dst4);

                auto inf_max = dp1 + dp2 + dp3 + dp4;

                if (inf_max < 0.000001f && inf_max > -0.000001f)
                    return color1.get().x();

                auto inf1 = dp1 / inf_max;
                auto inf2 = dp2 / inf_max;
                auto inf3 = dp3 / inf_max;
                auto inf4 = dp4 / inf_max;

                return color1.get().x() * inf1 + color2.get().x() * inf2 + color3.get().x() * inf3 +
                       color4.get().x() * inf4;
            };

        static constexpr auto make_vertex = [](const grx_float_color_map_r& hmap,
                                               float                        max_height,
                                               const vec2f&                 size_f,
                                               const vec2u&                 vert_idxs,
                                               const vec2u&                 vert_count,
                                               const vec2f&                 size,
                                               auto&&                       curve_func) {
            auto f  = size * vec2f(vert_idxs) / vec2f(vert_count);
            auto fy = max_height * take_color(hmap, size_f, f, curve_func);
            return vec{f.x(), fy, f.y()};
        };

        auto  size_f     = vec2f(height_map.size() - vec2u{1, 1}) / size;
        auto& vert_count = res.vertex_count;
        auto  uv_max     = vec2f(vert_count - vec2u{1, 1});

        for (auto [x, y] : dimensional_seq(vert_count))
            res.vertices.push_back({make_vertex(height_map, max_height, size_f, {x, y}, vert_count, size, curve_func),
                                       vec{float(x), float(y)} / uv_max});

        for (auto [x, y] : dimensional_seq(vert_count - vec2u{1, 1})) {
            res.faces.push_back({y * vert_count.x() + x, (y + 1) * vert_count.x() + x, y * vert_count.x() + x + 1});
            auto& f1      = res.faces.back();
            auto  face_id = u32(res.faces.size() - 1);
            f1.recalc_normal(res.vertices);
            res.vertices[f1.a].internal_face_id = face_id;
            res.vertices[f1.b].internal_face_id = face_id;
            res.vertices[f1.c].internal_face_id = face_id;

            res.faces.push_back(
                {y * vert_count.x() + x + 1, (y + 1) * vert_count.x() + x, (y + 1) * vert_count.x() + x + 1});
            auto& f2 = res.faces.back();
            face_id  = u32(res.faces.size() - 1);
            f2.recalc_normal(res.vertices);
            res.vertices[f2.a].internal_face_id = face_id;
            res.vertices[f2.b].internal_face_id = face_id;
            res.vertices[f2.c].internal_face_id = face_id;
        }

        for (auto& v : res.vertices) v.normal.make_normalize();

        return res;
    }

    void recalc_normals_smooth() {
        for (auto& face : faces)
            face.set_zero_normal(vertices);
        for (auto& face : faces)
            face.recalc_normal(vertices);
        for (auto& v : vertices)
            v.normal.make_normalize();
    }

    void amplify(float force, float brush_size, const vec2f& position, auto attenuation_f) {
        auto  vert_f     = vec2f(vertex_count - vec{1U, 1U}) / size;
        auto  vertpos    = vert_f * position;
        auto  vertidx    = vec2u{u32(std::round(vertpos.x())), u32(std::round(vertpos.y()))};
        auto& start_vert = vertices[vertidx.y() * vertex_count.x() + vertidx.x()];
        auto  start_xz   = start_vert.position.xz();
        auto  halfbrush  = brush_size * 0.5f;
        /* For using faster magnitude2 */
        auto halfbrush_2 = halfbrush * halfbrush;

        auto idx_min = vertidx;
        auto idx_max = vertidx;

        /* TODO: simplify/optimize this */
        while (idx_max.x() < vertex_count.x() &&
               (start_xz - vertices[vertidx.y() * vertex_count.x() + idx_max.x()].position.xz())
                       .magnitude_2() <= halfbrush_2)
            ++idx_max.x();
        while (idx_min.x() < vertex_count.x() &&
               (start_xz - vertices[vertidx.y() * vertex_count.x() + idx_min.x()].position.xz())
                       .magnitude_2() <= halfbrush_2)
            --idx_min.x();
        while (idx_max.y() < vertex_count.y() &&
               (start_xz - vertices[idx_max.y() * vertex_count.x() + vertidx.x()].position.xz())
                       .magnitude_2() <= halfbrush_2)
            ++idx_max.y();
        while (idx_min.y() < vertex_count.y() &&
               (start_xz - vertices[idx_min.y() * vertex_count.x() + vertidx.x()].position.xz())
                       .magnitude_2() <= halfbrush_2)
            --idx_min.y();

        for (auto [x, y] : dimensional_seq(idx_min, idx_max)) {
            auto& v = vertices[y * vertex_count.x() + x];
            auto dist = (start_xz - v.position.xz()).magnitude();
            if (dist > halfbrush)
                continue;

            auto coeff = attenuation_f((halfbrush - dist) / halfbrush);
            v.position.y() += force * coeff;
        }

        recalc_normals_smooth();
    }

    [[nodiscard]]
    string to_wavefront() const {
        string res;

        res += "# vertices\n";
        for (auto& v : vertices)
            res += build_string("v "sv,
                                to_string(v.position.x()),
                                ' ',
                                to_string(v.position.y()),
                                ' ',
                                to_string(v.position.z()),
                                '\n');

        res += "\n# uvs\n";
        for (auto& v : vertices) res += build_string("vt "sv, to_string(v.uv.x()), ' ', to_string(v.uv.y()), '\n');

        res += "\n# normals\n";
        for (auto& v : vertices)
            res += build_string(
                "vn "sv, to_string(v.normal.x()), ' ', to_string(v.normal.y()), ' ', to_string(v.normal.z()), '\n');

        res += "\n# faces\n";
        for (auto& face : faces) {
            auto i1 = to_string(face.a + 1);
            auto i2 = to_string(face.b + 1);
            auto i3 = to_string(face.c + 1);
            res +=
                build_string("f "sv, i1, '/', i1, '/', i1, ' ', i2, '/', i2, '/', i2, ' ', i3, '/', i3, '/', i3, '\n');
        }

        return res;
    }

    [[nodiscard]]
    grx_cpu_mesh_group_t to_mesh() const {
        grx_cpu_mesh_group_t res;

        grx::vbo_vector_indices indices;
        for (auto& face : faces) {
            indices.push_back(face.a);
            indices.push_back(face.b);
            indices.push_back(face.c);
        }
        res.set<mesh_buf_tag::index>(move(indices));

        static constexpr auto transform_component = [](auto&& vertices, auto&& f) {
            using comp_t = decltype(f(vertices.front()));
            return vertices / core::transform<vector<comp_t>>(f);
        };

        res.set<mesh_buf_tag::position>(transform_component(vertices, [](auto&& v) { return v.position; }));
        res.set<mesh_buf_tag::normal>(transform_component(vertices, [](auto&& v) { return v.normal; }));
        res.set<mesh_buf_tag::uv>(transform_component(vertices, [](auto&& v) { return v.uv; }));

        auto tangents   = grx::vbo_vector_vec3f(vertices.size());
        auto bitangents = grx::vbo_vector_vec3f(vertices.size());

        auto aabb = grx::grx_aabb::maximized();

        for (u32 i = 0; auto& [tng, bitng, vertex] : zip_view(tangents, bitangents, vertices)) {
            auto& face = faces[vertex.internal_face_id];

            u32 idxs[3] = {face.a, face.b, face.c};

            auto a_num = u32(std::find(idxs, idxs + 3, i) - idxs);
            auto b_num = (a_num + 1) % 3;
            auto c_num = (b_num + 1) % 3;

            auto& a = vertex;
            auto& b = vertices[idxs[b_num]];
            auto& c = vertices[idxs[c_num]];

            auto e1 = b.position - a.position;
            auto e2 = c.position - a.position;
            auto u1 = b.uv - a.uv;
            auto u2 = c.uv - a.uv;

            auto f = 1.f / (u1.x() * u2.y() - u2.x() * u1.y());
            tng    = (vec3f::filled_with(u2.y()) * e1 - vec3f::filled_with(u1.y()) * e2) * f;
            bitng  = (vec3f::filled_with(-u2.x()) * e1 + vec3f::filled_with(u1.x()) * e2) * f;

            aabb.merge({a.position, a.position});

            ++i;
        }

        res.set<mesh_buf_tag::tangent>(move(tangents));
        res.set<mesh_buf_tag::bitangent>(move(bitangents));

        res.elements({grx::grx_mesh_element{u32(faces.size() * 3), u32(vertices.size()), 0, 0, aabb}});

        return res;
    }

private:
    vector<vertex_data> vertices;
    vector<face_data>   faces;
    vec2f               size;
    vec2u               vertex_count;
};

inline auto terrain_tanh(float x_magnifier = 3.f) {
    return [x_magnifier](float v) {
        return (std::tanh(v * x_magnifier * 2.f - x_magnifier) + 1.f) * 0.5f;
    };
}
}

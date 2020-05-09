#pragma once

#include <cstddef>
#include <vector>
#include <queue>

#include <core/types.hpp>
#include <core/assert.hpp>
#include <core/vec.hpp>
#include <core/helper_macros.hpp>
#include <core/aligned_allocator.hpp>
#include "../grx_types.hpp"

namespace grx {
    class frustum_storage {
        SINGLETON_IMPL(frustum_storage);

    public:
        using aabb_fast_vec = core::vector<grx_aabb_fast, core::aligned_allocator<grx_aabb_fast, 32>>;
        using aabb_fast_ids = core::vector<size_t>;
        using result_vec    = core::vector<int32_t, core::aligned_allocator<int32_t, 32>>;

        size_t new_get_id    (const grx_aabb_fast& aabb);
        size_t new_get_id    ();
        void   remove_id (size_t id);

        grx_aabb_fast& aabb_from_id(size_t id) {
            RASSERT(id < aabbs.size());
            return aabbs[id];
        }

        const grx_aabb_fast& aabb_from_id(size_t id) const {
            RASSERT(id < aabbs.size());
            return aabbs[id];
        }

        int32_t result_from_id(size_t id) const {
            ASSERT(id < results.size());
            return results[id];
        }

        void clear() {
            results.clear();
            aabbs.clear();
            free_aabbs.clear();
        }

        void calculate_culling(const grx_aabb_frustum_planes_fast& frustum);

    private:
        frustum_storage();

        static void frustum_test_mt(
                core::function<void(int32_t*, float*, float*, size_t)> func,
                int32_t* results,
                float* aabbs,
                float* frustum,
                size_t count,
                size_t nprocs);

        void frustum_test(size_t start, const grx_aabb_frustum_planes_fast& frustum) {
            for (size_t i = start; i < aabbs.size(); ++i) {
                int32_t pass = 0;

                for (auto& plane : frustum.as_array)
                    pass |= (std::max(aabbs[i].min.x() * plane.x(), aabbs[i].max.x() * plane.x()) +
                             std::max(aabbs[i].min.y() * plane.y(), aabbs[i].max.y() * plane.y()) +
                             std::max(aabbs[i].min.z() * plane.z(), aabbs[i].max.z() * plane.z()) + plane.w()) <= 0;
                results[i] = pass;
            }
            //core::printline("results: {}", results);
        }

    private:
        aabb_fast_vec aabbs;
        aabb_fast_ids free_aabbs;
        result_vec    results;
    };


    inline auto& grx_frustum_mgr() {
        return frustum_storage::instance();
    }


    class grx_aabb_culling_proxy {
    public:
        grx_aabb_culling_proxy() {
            id = grx_frustum_mgr().new_get_id();
        }

        grx_aabb_culling_proxy(const core::vec3f& min, const core::vec3f& max) {
            id = grx_frustum_mgr().new_get_id(grx_aabb_fast{min, max});
        }

        ~grx_aabb_culling_proxy() {
            grx_frustum_mgr().remove_id(id);
        }

        grx_aabb_fast& aabb() {
            return grx_frustum_mgr().aabb_from_id(id);
        }

        const grx_aabb_fast& aabb() const {
            return grx_frustum_mgr().aabb_from_id(id);
        }

        bool is_visible() const {
            return grx_frustum_mgr().result_from_id(id) == 0;
        }

        grx_aabb_culling_proxy(const grx_aabb_culling_proxy& p) {
            id = grx_frustum_mgr().new_get_id(p.aabb());
        }

        grx_aabb_culling_proxy(grx_aabb_culling_proxy&& p): id(p.id) {
            p.id = std::numeric_limits<size_t>::max();
        }

    private:
        size_t id;
    };

} // namespace grx


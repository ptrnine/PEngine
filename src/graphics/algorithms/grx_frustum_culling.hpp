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
        ~frustum_storage() noexcept = default;

        using aabb_fast_vec = core::vector<grx_aabb_fast, core::aligned_allocator<grx_aabb_fast, 32>>; // NOLINT
        using aabb_fast_ids = core::vector<size_t>;
        using result_vec    = core::vector<int32_t, core::aligned_allocator<int32_t, 32>>; // NOLINT

        size_t new_get_id    (const grx_aabb_fast& aabb);
        size_t new_get_id    ();
        void   remove_id (size_t id);

        [[nodiscard]]
        grx_aabb_fast& aabb_from_id(size_t id) {
            RASSERT(id < aabbs.size());
            return aabbs[id];
        }

        [[nodiscard]]
        const grx_aabb_fast& aabb_from_id(size_t id) const {
            RASSERT(id < aabbs.size());
            return aabbs[id];
        }

        [[nodiscard]]
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
                core::function<void(void*, void*, void*, size_t)> func,
                void*  results,
                void*  aabbs,
                void*  frustum,
                size_t count,
                size_t nprocs);

        void frustum_test(size_t start, const grx_aabb_frustum_planes_fast& frustum) {
            for (size_t i = start; i < aabbs.size(); ++i) {
                int32_t pass = 0;

                for (auto& plane : frustum.as_array) // NOLINT
                    pass |= (std::max(aabbs[i].min.x() * plane.x(), aabbs[i].max.x() * plane.x()) +
                             std::max(aabbs[i].min.y() * plane.y(), aabbs[i].max.y() * plane.y()) +
                             std::max(aabbs[i].min.z() * plane.z(), aabbs[i].max.z() * plane.z()) + plane.w()) <= 0;
                results[i] = pass;
            }
        }

    private:
        result_vec    results;
        aabb_fast_vec aabbs;
        aabb_fast_ids free_aabbs;
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

        [[nodiscard]]
        const grx_aabb_fast& aabb() const {
            return grx_frustum_mgr().aabb_from_id(id);
        }

        [[nodiscard]]
        bool is_visible() const {
            return grx_frustum_mgr().result_from_id(id) == 0;
        }

        grx_aabb_culling_proxy(const grx_aabb_culling_proxy& p) {
            id = grx_frustum_mgr().new_get_id(p.aabb());
        }

        grx_aabb_culling_proxy& operator=(const grx_aabb_culling_proxy& p) {
            if (id != p.id) {
                grx_frustum_mgr().remove_id(id);
                id = grx_frustum_mgr().new_get_id(p.aabb());
            }
            return *this;
        }

        grx_aabb_culling_proxy(grx_aabb_culling_proxy&& p) noexcept: id(p.id) {
            p.id = std::numeric_limits<size_t>::max();
        }

        grx_aabb_culling_proxy& operator=(grx_aabb_culling_proxy&& p) noexcept {
            id = p.id;
            p.id = std::numeric_limits<size_t>::max();
            return *this;
        }

    private:
        size_t id;
    };

} // namespace grx


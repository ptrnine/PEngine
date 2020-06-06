#pragma once

#include <cstdlib>
#include "types.hpp"

namespace core {

    template <typename T, size_t N>
    class aligned_allocator {
    public:
        using value_type      = T;
        using size_type       = size_t;
        using difference_type = ptrdiff_t;
        using pointer         = T*;
        using const_pointer   = const T*;
        using reference       = T&;
        using const_reference = const T&;

        aligned_allocator () noexcept = default;
        ~aligned_allocator() noexcept = default;

        aligned_allocator (const aligned_allocator&) = default;
        aligned_allocator& operator= (const aligned_allocator&) = default;

        aligned_allocator(aligned_allocator&&) = delete;
        aligned_allocator& operator= (aligned_allocator&&) = delete;

        template <typename T2>
        explicit aligned_allocator(const aligned_allocator<T2, N>&) noexcept {}


        pointer adress(reference r) {
            return &r;
        }

        const_pointer adress(const_reference r) {
            return &r;
        }

// TODO: Move to platform_dependent
#ifdef _WIN32

        pointer allocate(size_t n) {
            auto ptr = static_cast<pointer>(_aligned_malloc(n * sizeof(value_type), N));

            if (!ptr)
                throw std::bad_alloc();

            return ptr;
        }

        void deallocate(pointer p, size_t) {
            _aligned_free(p);
        }

#else

        pointer allocate(size_t n) {
            auto ptr = static_cast<pointer>(std::aligned_alloc(N, n * sizeof(value_type))); // NOLINT

            if (!ptr)
                throw std::bad_alloc();

            return ptr;
        }

        void deallocate(pointer p, size_t) {
            std::free(p); // NOLINT
        }

#endif

        template <typename P, typename... Ts>
        void construct(P* p, Ts&&... values) {
            ::new (reinterpret_cast<void*>(p)) value_type(std::forward<Ts>(values)...); // NOLINT
        }

        void destroy(pointer p) {
            p->~value_type();
        }

        template <typename T2>
        struct rebind {
            using other = aligned_allocator<T2, N>;
        };

        bool operator==(const aligned_allocator&) const {
            return true;
        }

        bool operator!=(const aligned_allocator& other) const {
            return !(*this == other);
        }
    };

} // namespace core


#pragma once

#include <core/types.hpp>
#include <core/vec.hpp>
#include "grx_types.hpp"

namespace grx {
    // OpenGL backend
    void _grx_render_target_tuple_init(
            core::pair<grx_color_fmt, grx_filtering>* settings,
            const core::vec2i& size,
            uint* framebuffer_ids,
            uint* texture_ids,
            uint* depthbuffer_ids,
            size_t count
    );

    void _grx_render_target_tuple_delete(
            uint* framebuffer_ids,
            uint* texture_ids,
            uint* depthbuffer_ids,
            size_t count
    );

    void _grx_render_target_tuple_bind(uint framebuffer_id, const core::vec2i& size);
    void _grx_render_target_tuple_active_texture(uint texture_id);
    void _grx_render_target_tuple_clear();

    template <RenderTargetSettings... Ts>
    class grx_render_target_tuple {
    public:
        static constexpr size_t size() {
            return sizeof...(Ts);
        }

        static core::shared_ptr<grx_render_target_tuple<Ts...>>
        create_shared(const core::vec2i& isize) {
            return core::make_shared<grx_render_target_tuple<Ts...>>(isize);
        }

        grx_render_target_tuple(const core::vec2i& isize): _size(isize) {
            core::array settings = {core::pair{Ts::color_fmt, Ts::filtering}...};
            _grx_render_target_tuple_init(
                settings.data(),
                _size,
                _framebuffer_ids.data(),
                _texture_ids.data(),
                _depthbuffer_ids.data(),
                size()
            );
        }

        ~grx_render_target_tuple() {
            _grx_render_target_tuple_delete(
                _framebuffer_ids.data(),
                _texture_ids.data(),
                _depthbuffer_ids.data(),
                size()
            );
        }

        grx_render_target_tuple(grx_render_target_tuple&& t) noexcept:
            _framebuffer_ids(t._framebuffer_ids),
            _texture_ids    (t._texture_ids),
            _depthbuffer_ids(t._depthbuffer_ids),
            _size           (t._size)
        {
            for (auto& f : t._framebuffer_ids)
                f = std::numeric_limits<uint>::max();
            for (auto& f : t._texture_ids)
                f = std::numeric_limits<uint>::max();
            for (auto& f : t._depthbuffer_ids)
                f = std::numeric_limits<uint>::max();
        }

        grx_render_target_tuple& operator= (grx_render_target_tuple&& t) noexcept {
            _framebuffer_ids = t._framebuffer_ids;
            _texture_ids     = t._texture_ids;
            _depthbuffer_ids = t._depthbuffer_ids;
            _size            = t._size;

            for (auto& f : t._framebuffer_ids)
                f = std::numeric_limits<uint>::max();
            for (auto& f : t._texture_ids)
                f = std::numeric_limits<uint>::max();
            for (auto& f : t._depthbuffer_ids)
                f = std::numeric_limits<uint>::max();

            return *this;
        }

        //void bind();

        template <size_t S>
        [[nodiscard]]
        uint texture_id() const {
            return std::get<S>(_texture_ids);
        }

        template <size_t S>
        void activate_texture() const {
            _grx_render_target_tuple_active_texture(texture_id<S>());
        }

        template <size_t S>
        void bind() {
            _grx_render_target_tuple_bind(std::get<S>(_framebuffer_ids), _size);
        }

        template <size_t S>
        void bind_and_clear() {
            _grx_render_target_tuple_bind(std::get<S>(_framebuffer_ids), _size);
            _grx_render_target_tuple_clear();
        }


        template <size_t S1, size_t S2>
        void swap_targets() {
            static_assert(std::is_same_v<
                    std::tuple_element_t<S1, core::tuple<Ts...>>,
                    std::tuple_element_t<S2, core::tuple<Ts...>>>);

            std::swap(std::get<S1>(_framebuffer_ids), std::get<S2>(_framebuffer_ids));
            std::swap(std::get<S1>(_texture_ids),     std::get<S2>(_texture_ids));
            std::swap(std::get<S1>(_depthbuffer_ids), std::get<S2>(_depthbuffer_ids));
        }

    private:
        core::array<uint, size()> _framebuffer_ids = { (Ts(), std::numeric_limits<uint>::max())... };
        core::array<uint, size()> _texture_ids     = { (Ts(), std::numeric_limits<uint>::max())... };
        core::array<uint, size()> _depthbuffer_ids = { (Ts(), std::numeric_limits<uint>::max())... };
        core::vec2i _size;
    };
} // namespace grx

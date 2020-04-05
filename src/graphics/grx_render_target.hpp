#pragma once

#include <core/vec.hpp>

namespace grx {
    class grx_render_target {
    public:
        grx_render_target(const core::vec2i& size);
        ~grx_render_target();

        grx_render_target(grx_render_target&& t):
            _framebuffer_id(t._framebuffer_id),
            _texture_id    (t._texture_id),
            _depthbuffer_id(t._depthbuffer_id),
            _size          (t._size)
        {
            t._framebuffer_id = std::numeric_limits<uint>::max();
            t._texture_id     = std::numeric_limits<uint>::max();
            t._depthbuffer_id = std::numeric_limits<uint>::max();
        }

        grx_render_target& operator= (grx_render_target&& t) {
            _framebuffer_id = t._framebuffer_id;
            _texture_id     = t._texture_id;
            _depthbuffer_id = t._depthbuffer_id;
            _size           = t._size;

            t._framebuffer_id = std::numeric_limits<uint>::max();
            t._texture_id     = std::numeric_limits<uint>::max();
            t._depthbuffer_id = std::numeric_limits<uint>::max();

            return *this;
        }

        void bind();

        [[nodiscard]]
        uint texture_id() const {
            return _texture_id;
        }

    protected:
        // Only grx_window can break RAII
        friend class grx_window;
        grx_render_target() = default;

    private:
        uint _framebuffer_id = std::numeric_limits<uint>::max();
        uint _texture_id     = std::numeric_limits<uint>::max();
        uint _depthbuffer_id = std::numeric_limits<uint>::max();
        core::vec2i _size;
    };
} // namespace grx

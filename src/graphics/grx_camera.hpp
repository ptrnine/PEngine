#pragma once

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/constants.hpp>

#include <core/helper_macros.hpp>
#include <core/types.hpp>
#include <core/vec.hpp>

#include "grx_types.hpp"
#include "grx_camera_manipulator.hpp"

namespace grx {
    class grx_camera {
    public:
        static constexpr float FOV_MIN    = 10.f;
        static constexpr float FOV_MAX    = 180.f;
        static constexpr float PITCH_LOCK = glm::half_pi<float>() - 0.05f;
        static constexpr float ROLL_LOCK  = glm::half_pi<float>() - 0.1f;

    public:
        static core::shared_ptr<grx_camera>
        make_shared(const core::vec3f& pos = { 0.f, 0.f, 0.f },
                    float aspect_ratio = 16.f / 9.f,
                    float fov = 73.f,
                    float z_near = 1.1f,
                    float z_far = 1536.f)
        {
            return core::shared_ptr<grx_camera>(
                    new grx_camera(pos, aspect_ratio, fov, z_near, z_far));
        }

        void look_at(const core::vec3f& pos);
        void calc_orientation();
        void calc_view_projection();
        void update(class grx_window* window = nullptr);
        auto extract_frustum() const -> grx_aabb_frustum_planes_fast;

        template <typename T, typename... Ts>
        void create_camera_manipulator(Ts&&... args) {
            _camera_manipulator = core::unique_ptr<T>(new T(core::forward<Ts>(args)...));
        }

    protected:
        grx_camera(const core::vec3f& pos,
                   float aspect_ratio,
                   float fov,
                   float z_near,
                   float z_far
        ): _pos(pos), _aspect_ratio(aspect_ratio), _fov(fov), _z_near(z_near), _z_far(z_far) {}

    private:
        core::vec3f _pos;
        core::vec3f _dir    = { 0.f, 0.f, 1.f };
        core::vec3f _right  = { 1.f, 0.f, 0.f };
        core::vec3f _up     = { 0.f, 1.f, 0.f };

        glm::mat4 _orientation = glm::mat4(1.f);
        glm::mat4 _view        = glm::mat4(1.f);
        glm::mat4 _projection  = glm::mat4(1.f);

        float _aspect_ratio;
        float _fov;
        float _z_near;
        float _z_far;

        float _yaw   = 0.f;
        float _pitch = 0.f;
        float _roll  = 0.f;

        core::unique_ptr<grx_camera_manipulator> _camera_manipulator;

    public:
        DECLARE_GET(view)
        DECLARE_GET(projection)
        DECLARE_GET(orientation)
        DECLARE_VAL_GET(fov)
        DECLARE_VAL_GET(aspect_ratio)
        DECLARE_VAL_GET(z_near)
        DECLARE_VAL_GET(z_far)

        [[nodiscard]]
        core::vec3f ypr() const {
            return core::vec{_yaw, _pitch, _roll};
        }

        [[nodiscard]]
        glm::mat4 view_projection() const {
            return _projection * _view;
        }

        [[nodiscard]]
        core::vec3f position() const {
            return _pos;
        }

        [[nodiscard]]
        core::vec3f directory() const {
            return _dir;
        }

        [[nodiscard]]
        core::vec3f up() const {
            return _up;
        }

        [[nodiscard]]
        core::vec3f right() const {
            return _right;
        }

        void set_position(const core::vec3f& value) {
            _pos = value;
        }

        void set_fov(float value) {
            _fov = std::clamp(value, FOV_MIN, FOV_MAX);
        }

        void set_aspect_ratio(float value) {
            _aspect_ratio = value;
        }

        void set_z_near(float value) {
            _z_near = value;
        }

        void set_z_far(float value) {
            _z_far = value;
        }
    };
} // namespace grx

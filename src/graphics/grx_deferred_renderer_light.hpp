#pragma once

#include <core/helper_macros.hpp>
#include "graphics/algorithms/grx_frustum_culling.hpp"
#include "grx_shader_tech.hpp"
#include "grx_types.hpp"
#include "grx_cascaded_shadow_mapping_tech.hpp"
#include "grx_mesh_gen.hpp"
#include "grx_g_buffer.hpp"

namespace grx
{
struct grx_ds_light_type {
    enum type {
        no_light = -1,
        directional = 0,
        point = 1,
        spot = 2
    };
};

struct grx_ds_light_base {
    vec3f color             = {1.f, 1.f, 1.f};
    float ambient_intensity = 0.1f;
    float diffuse_intensity = 0.9f;
};

struct grx_ds_light_base_uniform {
    grx_uniform<vec3f> color;
    grx_uniform<float> ambient_intensity;
    grx_uniform<float> diffuse_intensity;
};

struct grx_ds_dir_light : grx_ds_light_base {
    vec3f direction = {0.f, 0.f, -1.f};
};

struct grx_ds_dir_light_uniform : grx_ds_light_base_uniform {
    grx_ds_dir_light_uniform(core::shared_ptr<grx_shader_program>& sp) {
        color             = sp->get_uniform_unwrap<vec3f>("dir_light.base.color");
        ambient_intensity = sp->get_uniform_unwrap<float>("dir_light.base.ambient_intensity");
        diffuse_intensity = sp->get_uniform_unwrap<float>("dir_light.base.diffuse_intensity");
        direction         = sp->get_uniform_unwrap<vec3f>("dir_light.direction");
    }
    grx_ds_dir_light_uniform& operator=(const grx_ds_dir_light& l) {
        color             = l.color;
        ambient_intensity = l.ambient_intensity;
        diffuse_intensity = l.diffuse_intensity;
        direction         = l.direction;

        return *this;
    }
    grx_uniform<vec3f> direction;
};

using light_mesh_t = grx_vbo_tuple<vbo_vector_indices, vbo_vector_vec3f>;

class grx_ds_point_light : public grx_ds_light_base {
public:
    grx_ds_point_light();
    grx_ds_point_light(const grx_ds_point_light&) = delete;
    grx_ds_point_light(grx_ds_point_light&&) = default;
    grx_ds_point_light& operator=(const grx_ds_point_light&) = delete;
    grx_ds_point_light& operator=(grx_ds_point_light&&) = default;
    virtual ~grx_ds_point_light() = default;

    vec3f position              = {0.f, 0.f, 0.f};
    float attenuation_constant  = 1.f;
    float attenuation_linear    = 0.f;
    float attenuation_quadratic = 0.f;

    light_mesh_t           mesh_;
    grx_aabb               aabb_;
    grx_aabb_culling_proxy aabb_proxy_;
    glm::mat4              model_mat_;

    [[nodiscard]]
    float max_ray_length() const;

    [[nodiscard]]
    light_mesh_t& mesh() {
        return mesh_;
    }

    virtual void calc_model_matrix();

    const glm::mat4& model_matrix() const {
        return model_mat_;
    }
};

struct grx_ds_point_light_uniform : grx_ds_light_base_uniform {
    grx_ds_point_light_uniform(core::shared_ptr<grx_shader_program>& sp, const core::string& name = "point_light") {
        color                 = sp->get_uniform_unwrap<vec3f>(name + ".base.color");
        ambient_intensity     = sp->get_uniform_unwrap<float>(name + ".base.ambient_intensity");
        diffuse_intensity     = sp->get_uniform_unwrap<float>(name + ".base.diffuse_intensity");
        position              = sp->get_uniform_unwrap<vec3f>(name + ".position");
        attenuation_constant  = sp->get_uniform_unwrap<float>(name + ".attenuation_constant");
        attenuation_linear    = sp->get_uniform_unwrap<float>(name + ".attenuation_linear");
        attenuation_quadratic = sp->get_uniform_unwrap<float>(name + ".attenuation_quadratic");
    }

    grx_ds_point_light_uniform& operator=(const grx_ds_point_light& l) {
        color                 = l.color;
        ambient_intensity     = l.ambient_intensity;
        diffuse_intensity     = l.diffuse_intensity;
        position              = l.position;
        attenuation_constant  = l.attenuation_constant;
        attenuation_linear    = l.attenuation_linear;
        attenuation_quadratic = l.attenuation_quadratic;

        return *this;
    }

    grx_uniform<vec3f> position;
    grx_uniform<float> attenuation_constant;
    grx_uniform<float> attenuation_linear;
    grx_uniform<float> attenuation_quadratic;
};

class grx_ds_spot_light : public grx_ds_point_light {
public:
    grx_ds_spot_light();

    vec3f direction = {0.f, 0.f, -1.f}; // NOLINT

    [[nodiscard]]
    float cutoff() const {
        return cutoff_;
    }

    void beam_angle(float degree) {
        cutoff_ = std::cos(core::angle::radian(degree) * 0.5f);
        recalc_mesh();
    }

    [[nodiscard]]
    float beam_angle() const {
        return core::angle::degree(std::acos(cutoff_) * 2.0f);
    }

    void shape(const core::vector<vec2f>& shape) {
        spot_shape = shape;
        recalc_mesh();
    }

    [[nodiscard]]
    const core::vector<vec2f>& shape() const {
        return spot_shape;
    }

    void calc_model_matrix() final;

private:
    void recalc_mesh();

    core::vector<vec2f> spot_shape = {{-1.f, -1.f}, {-1.f, 1.f}, {1.f, 1.f}, {1.f, -1.f}};
    float cutoff_ = 0.4f;
};

struct grx_ds_spot_light_uniform : grx_ds_point_light_uniform {
    grx_ds_spot_light_uniform(core::shared_ptr<grx_shader_program>& sp):
        grx_ds_point_light_uniform(sp, "spot_light.base") {
        direction = sp->get_uniform_unwrap<vec3f>("spot_light.direction");
        cutoff    = sp->get_uniform_unwrap<float>("spot_light.cutoff");
    }

    grx_ds_spot_light_uniform& operator=(const grx_ds_spot_light& l) {
        grx_ds_point_light_uniform::operator=(l);
        direction = l.direction;
        cutoff    = l.cutoff();
        return *this;
    }

    grx_uniform<vec3f> direction;
    grx_uniform<float> cutoff;
};


class grx_ds_light_mgr;

struct grx_ds_light_param_uniform {
    grx_ds_light_param_uniform(core::shared_ptr<grx_shader_program>& sp) {
        specular_power     = sp->get_uniform_unwrap<float>    ("specular_power");
        specular_intensity = sp->get_uniform_unwrap<float>    ("specular_intensity");
        eye_pos_ws         = sp->get_uniform_unwrap<vec3f>    ("eye_pos_ws");
        light_type         = sp->get_uniform_unwrap<int>      ("light_type");
        overlay_MVP        = sp->get_uniform_unwrap<glm::mat4>("MVP");
    }

    grx_ds_light_param_uniform& operator=(const grx_ds_light_mgr&);

    void set_light_type(grx_ds_light_type::type ilight_type) {
        light_type = ilight_type;
    }

    void set_overlay_mvp(const glm::mat4& matrix) {
        overlay_MVP = matrix;
    }

    grx_uniform<float>     specular_power;
    grx_uniform<float>     specular_intensity;
    grx_uniform<vec3f>     eye_pos_ws;
    grx_uniform<int>       light_type;
    grx_uniform<glm::mat4> overlay_MVP;
};

template <typename T>
concept LightListIter = std::same_as<T, std::list<grx_ds_point_light>::iterator> ||
                        std::same_as<T, std::list<grx_ds_spot_light>::iterator>;

class grx_ds_dir_light_provider;

template <LightListIter IterT = std::list<grx_ds_point_light>::iterator>
class grx_ds_point_light_provider;
class grx_ds_spot_light_provider;

class grx_ds_light_mgr : public std::enable_shared_from_this<grx_ds_light_mgr> {
public:
    friend class grx_ds_dir_light_provider;
    template <LightListIter>
    friend class grx_ds_point_light_provider;
    friend class grx_ds_spot_light_provider;

    grx_ds_light_mgr(const grx_ds_light_mgr&) = delete;
    grx_ds_light_mgr& operator=(const grx_ds_light_mgr&) = delete;
    grx_ds_light_mgr(grx_ds_light_mgr&&) = default;
    grx_ds_light_mgr& operator=(grx_ds_light_mgr&&) = default;

    struct uniforms_t {
        uniforms_t(core::shared_ptr<grx_shader_program>& shader):
            param(shader), dir_light(shader), point_light(shader), spot_light(shader) {}

        grx_ds_light_param_uniform param;
        grx_ds_dir_light_uniform   dir_light;
        grx_ds_point_light_uniform point_light;
        grx_ds_spot_light_uniform  spot_light;
    };

    grx_ds_light_mgr(core::constructor_accessor<grx_ds_light_mgr>::cref, vec2u gbuffer_size):
        gbuf_(gbuffer_size),
        sphere_program(grx_shader_program::create_shared(
            grx_shader<shader_type::vertex>("uniform mat4 MVP;"
                                            "in vec3 position_ms;"
                                            "void main() {"
                                            "    gl_Position = MVP * vec4(position_ms, 1.0);"
                                            "}"
                                            ))) {}

    ~grx_ds_light_mgr() = default;

    static core::shared_ptr<grx_ds_light_mgr> create_shared(vec2u gbuffer_size) {
        return std::make_shared<grx_ds_light_mgr>(core::constructor_accessor<grx_ds_light_mgr>(), gbuffer_size);
    }

    grx_ds_dir_light_provider     create_dir_light();
    grx_ds_spot_light_provider    create_spot_light();
    grx_ds_point_light_provider<> create_point_light();

    void light_pass(core::shared_ptr<grx_shader_program>& program, vec3f eye_position, const glm::mat4& view_projection);
    void light_pass(grx_shader_tech& tech, grx_mesh_type type, vec3f eye_position, const glm::mat4& view_projection);
    void start_geometry_pass();

    void present(vec2u output_fbo_size);

private:
    void put_back_dir_light() {
        enable_dir_light_ = false;
    }

    void remove_point_light(core::list<grx_ds_point_light>::iterator iter) {
        point_lights_.erase(iter);
    }

    void remove_spot_light(core::list<grx_ds_spot_light>::iterator iter) {
        spot_lights_.erase(iter);
    }

    grx_g_buffer gbuf_;
    core::shared_ptr<grx_shader_program> sphere_program;

    core::hash_map<grx_shader_program*, uniforms_t> cached_uniforms_;
    grx_ds_dir_light dir_light_;

    core::list<grx_ds_point_light> point_lights_;
    core::list<grx_ds_spot_light>  spot_lights_;

    vec3f eye_position_       = {0.f, 0.f, 0.f};
    float specular_power_     = 0.f;
    float specular_intensity_ = 0.f;
    bool  enable_dir_light_   = false;
    bool  enable_debug_draw_  = false;

public:
    void enable_debug_draw(bool value = true) { enable_debug_draw_ = value; }
    void disable_debug_draw() { enable_debug_draw(false); }

    void  specular_power    (float value) { specular_power_ = value; }
    void  specular_intensity(float value) { specular_intensity_ = value; }
    float specular_power    () const { return specular_power_; }
    float specular_intensity() const { return specular_intensity_; }
    vec3f eye_position      () const { return eye_position_; }

    void enable_dir_light (bool value = true) { enable_dir_light_ = value; }
    void disable_dir_light() { enable_dir_light(false); }

    bool has_dir_light() const {
        return enable_dir_light_;
    }

    grx_ds_dir_light& dir_light() {
        return dir_light_;
    }
};

class grx_ds_light_provider_base {
public:
    grx_ds_light_provider_base(core::weak_ptr<grx_ds_light_mgr> mgr): mgr_(core::move(mgr)) {}

    core::shared_ptr<grx_ds_light_mgr> try_mgr() {
        return mgr_.lock();
    }

    [[nodiscard]]
    core::shared_ptr<const grx_ds_light_mgr> try_mgr() const {
        return mgr_.lock();
    }

    grx_ds_light_mgr& mgr() {
        if (auto mgr = mgr_.lock())
            return *mgr;
        else
            throw std::runtime_error("Deferred light manager was destroyed!");
    }

    [[nodiscard]]
    const grx_ds_light_mgr& mgr() const {
        if (auto mgr = mgr_.lock())
            return *mgr;
        else
            throw std::runtime_error("Deferred light manager was destroyed!");
    }

protected:
    core::weak_ptr<grx_ds_light_mgr> mgr_; // NOLINT
};

class grx_ds_dir_light_provider : public grx_ds_light_provider_base {
public:
    grx_ds_dir_light_provider(core::weak_ptr<grx_ds_light_mgr> mgr):
        grx_ds_light_provider_base(core::move(mgr)) {}

    grx_ds_dir_light_provider(const grx_ds_dir_light_provider&) = delete;
    grx_ds_dir_light_provider& operator=(const grx_ds_dir_light_provider&) = delete;
    grx_ds_dir_light_provider(grx_ds_dir_light_provider&&) noexcept = default;
    grx_ds_dir_light_provider& operator=(grx_ds_dir_light_provider&&) noexcept = default;

    ~grx_ds_dir_light_provider() {
        if (auto mgr = mgr_.lock())
            mgr->put_back_dir_light();
    }

#define _DEF_GETSET(FIELD_TYPE, FIELD_NAME)                                                                            \
    void FIELD_NAME(FIELD_TYPE value) {                                                                                \
        if (auto mgr = mgr_.lock())                                                                                    \
            mgr->dir_light().FIELD_NAME = value;                                                                       \
        else                                                                                                           \
            throw std::runtime_error("Deferred light manager was destroyed!");                                         \
    }                                                                                                                  \
    [[nodiscard]] std::decay_t<FIELD_TYPE> FIELD_NAME() const {                                                        \
        if (auto mgr = mgr_.lock())                                                                                    \
            return mgr->dir_light().FIELD_NAME;                                                                        \
        else                                                                                                           \
            throw std::runtime_error("Deferred light manager was destroyed!");                                         \
    }                                                                                                                  \
    [[nodiscard]] core::try_opt<std::decay_t<FIELD_TYPE>> try_##FIELD_NAME() const {                                   \
        if (auto mgr = mgr_.lock())                                                                                    \
            return mgr->dir_light().FIELD_NAME;                                                                        \
        else                                                                                                           \
            return std::runtime_error("Deferred light manager was destroyed!");                                        \
    }

    _DEF_GETSET(const vec3f&, color)
    _DEF_GETSET(const vec3f&, direction)
    _DEF_GETSET(float, ambient_intensity)
    _DEF_GETSET(float, diffuse_intensity)

#undef _DEF_GETSET
};

template <LightListIter IterT>
class grx_ds_point_light_provider : public grx_ds_light_provider_base {
public:
    grx_ds_point_light_provider(const core::weak_ptr<grx_ds_light_mgr>& mgr, IterT iter):
        grx_ds_light_provider_base(mgr), iter_(iter) {}

    grx_ds_point_light_provider(const grx_ds_point_light_provider&) = delete;
    grx_ds_point_light_provider& operator=(const grx_ds_point_light_provider&) = delete;
    grx_ds_point_light_provider(grx_ds_point_light_provider&&) noexcept = default;
    grx_ds_point_light_provider& operator=(grx_ds_point_light_provider&&) noexcept = default;

    ~grx_ds_point_light_provider() {
        if (auto mgr = mgr_.lock()) {
            if constexpr (std::is_same_v<IterT, std::list<grx_ds_point_light>::iterator>)
                mgr->remove_point_light(iter_);
            else if constexpr (std::is_same_v<IterT, std::list<grx_ds_spot_light>::iterator>)
                mgr->remove_spot_light(iter_);
        }
    }

protected:
    void check_mgr() const {
        if (auto mgr = mgr_.lock(); !mgr)
            throw std::runtime_error("Deferred light manager was destroyed");
    }

public:
#define _DEF_GETSET(FIELD_TYPE, FIELD_NAME)                                                                            \
    void FIELD_NAME(FIELD_TYPE value) {                                                                                \
        check_mgr();                                                                                                   \
        iter_->FIELD_NAME = value;                                                                                     \
    }                                                                                                                  \
    [[nodiscard]] std::decay_t<FIELD_TYPE> FIELD_NAME() const {                                                        \
        check_mgr();                                                                                                   \
        return iter_->FIELD_NAME;                                                                                      \
    }                                                                                                                  \
    [[nodiscard]] core::try_opt<std::decay_t<FIELD_TYPE>> try_##FIELD_NAME() const {                                   \
        if (auto mgr = mgr_.lock())                                                                                    \
            return FIELD_NAME();                                                                                       \
        else                                                                                                           \
            return std::runtime_error("Deferred light manager was destroyed");                                         \
    }

#define _DEF_GET_FUNC(FIELD_TYPE, FIELD_NAME)                                                                          \
    void FIELD_NAME(FIELD_TYPE value) {                                                                                \
        check_mgr();                                                                                                   \
        iter_->FIELD_NAME(value);                                                                                      \
    }                                                                                                                  \
    [[nodiscard]] std::decay_t<FIELD_TYPE> FIELD_NAME() const {                                                        \
        check_mgr();                                                                                                   \
        return iter_->FIELD_NAME();                                                                                    \
    }
#define _DEF_SET_FUNC(FIELD_TYPE, FIELD_NAME)                                                                          \
    [[nodiscard]] core::try_opt<std::decay_t<FIELD_TYPE>> try_##FIELD_NAME() const {                                   \
        if (auto mgr = mgr_.lock())                                                                                    \
            return FIELD_NAME();                                                                                       \
        else                                                                                                           \
            return std::runtime_error("Deferred light manager was destroyed");                                         \
    }
#define _DEF_GETSET_FUNC(FIELD_TYPE, FIELD_NAME) \
    _DEF_GET_FUNC(FIELD_TYPE, FIELD_NAME) _DEF_SET_FUNC(FIELD_TYPE, FIELD_NAME)

    _DEF_GETSET(const vec3f&, color)
    _DEF_GETSET(const vec3f&, position)
    _DEF_GETSET(float, ambient_intensity)
    _DEF_GETSET(float, diffuse_intensity)
    _DEF_GETSET(float, attenuation_constant)
    _DEF_GETSET(float, attenuation_linear)
    _DEF_GETSET(float, attenuation_quadratic)
    _DEF_GET_FUNC(float, max_ray_length)

protected:
    IterT iter_; // NOLINT
};

class grx_ds_spot_light_provider : public grx_ds_point_light_provider<std::list<grx_ds_spot_light>::iterator> {
public:
    grx_ds_spot_light_provider(const core::weak_ptr<grx_ds_light_mgr>& mgr,
                               core::list<grx_ds_spot_light>::iterator iter):
        grx_ds_point_light_provider<std::list<grx_ds_spot_light>::iterator>(mgr, iter) {}

    grx_ds_spot_light_provider(const grx_ds_spot_light_provider&) = delete;
    grx_ds_spot_light_provider& operator=(const grx_ds_spot_light_provider&) = delete;
    grx_ds_spot_light_provider(grx_ds_spot_light_provider&&) noexcept = default;
    grx_ds_spot_light_provider& operator=(grx_ds_spot_light_provider&&) noexcept = default;
    ~grx_ds_spot_light_provider() = default;

    _DEF_GETSET(const vec3f&, direction)
    _DEF_GETSET_FUNC(float, beam_angle)

    void shape(const core::vector<vec2f>& points) {
        if (auto mgr = mgr_.lock())
            iter_->shape(points);
        else
            throw std::runtime_error("Forward light manager was destroyed!");
    }
    [[nodiscard]]
    const core::vector<vec2f>& shape() const {
        if (auto mgr = mgr_.lock())
            return iter_->shape();
        else
            throw std::runtime_error("Forward light manager was destroyed!");
    }
    [[nodiscard]]
    core::try_opt<core::vector<vec2f>> try_shape() const {
        if (auto mgr = mgr_.lock())
            return iter_->shape();
        else
            return std::runtime_error("Forward light manager was destroyed!");
    }
};

#undef _DEF_GETSET
#undef _DEF_GETSET_FUNC

} // namespace grx

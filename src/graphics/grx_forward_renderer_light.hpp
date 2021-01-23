#pragma once

#include <core/helper_macros.hpp>
#include "grx_shader_tech.hpp"
#include "grx_types.hpp"
#include "grx_cascaded_shadow_mapping_tech.hpp"

#define PE_FORWARD_MAX_POINT_LIGHTS 16
#define PE_FORWARD_MAX_SPOT_LIGHTS  16

namespace grx
{
struct grx_light_base {
    vec3f color             = {1.f, 1.f, 1.f};
    float ambient_intensity = 0.1f;
    float diffuse_intensity = 0.9f;
};

struct grx_light_base_uniform {
    grx_uniform<vec3f> color;
    grx_uniform<float> ambient_intensity;
    grx_uniform<float> diffuse_intensity;
};

struct grx_dir_light : grx_light_base {
    vec3f direction = {0.f, 0.f, -1.f};
};

struct grx_dir_light_uniform : grx_light_base_uniform {
    grx_dir_light_uniform(grx_shader_program& sp) {
        color             = sp.get_uniform<vec3f>("dir_light.base.color");
        ambient_intensity = sp.get_uniform<float>("dir_light.base.ambient_intensity");
        diffuse_intensity = sp.get_uniform<float>("dir_light.base.diffuse_intensity");
        direction         = sp.get_uniform<vec3f>("dir_light.direction");
    }
    grx_dir_light_uniform& operator=(const grx_dir_light& l) {
        color             = l.color;
        ambient_intensity = l.ambient_intensity;
        diffuse_intensity = l.diffuse_intensity;
        direction         = l.direction;

        return *this;
    }
    grx_uniform<vec3f> direction;
};

struct grx_point_light : grx_light_base {
    vec3f position              = {0.f, 0.f, 0.f};
    float attenuation_constant  = 1.f;
    float attenuation_linear    = 0.f;
    float attenuation_quadratic = 0.f;
};

struct grx_point_light_uniform : grx_light_base_uniform {
    grx_point_light_uniform(grx_shader_program& sp, const core::string& name) {
        color                 = sp.get_uniform<vec3f>(name + ".base.color");
        ambient_intensity     = sp.get_uniform<float>(name + ".base.ambient_intensity");
        diffuse_intensity     = sp.get_uniform<float>(name + ".base.diffuse_intensity");
        position              = sp.get_uniform<vec3f>(name + ".position");
        attenuation_constant  = sp.get_uniform<float>(name + ".attenuation_constant");
        attenuation_linear    = sp.get_uniform<float>(name + ".attenuation_linear");
        attenuation_quadratic = sp.get_uniform<float>(name + ".attenuation_quadratic");
    }
    grx_point_light_uniform(grx_shader_program& sp, size_t num):
        grx_point_light_uniform(sp, "point_lights[" + std::to_string(num) + "]") {}

    grx_point_light_uniform& operator=(const grx_point_light& l) {
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

struct grx_spot_light : grx_point_light {
    vec3f direction = {0.f, 0.f, -1.f};
    float cutoff    = 0.4f;
    glm::mat4 VP; // NOLINT

    /**
     * @brief Build VP matrix from position and direction
     */
    void update_view_projection();
};

struct grx_spot_light_uniform : grx_point_light_uniform {
    grx_spot_light_uniform(grx_shader_program& sp, size_t num):
        grx_point_light_uniform(sp, "spot_lights[" + std::to_string(num) + "].base") {
        direction = sp.get_uniform<vec3f>(core::format("spot_lights[{}].direction", num));
        cutoff    = sp.get_uniform<float>(core::format("spot_lights[{}].cutoff", num));
        MVP       = sp.get_uniform<glm::mat4>("spot_MVP[" + std::to_string(num) + "]");
        texture   = sp.get_uniform<int>(core::format("spot_shadow_map[{}]", num));
    }

    grx_spot_light_uniform& operator=(const grx_spot_light& l) {
        grx_point_light_uniform::operator=(l);
        direction                        = l.direction;
        cutoff                           = l.cutoff;
        return *this;
    }

    grx_uniform<vec3f> direction;
    grx_uniform<float> cutoff;
    grx_uniform<glm::mat4> MVP;
    grx_uniform<int>       texture;
};

class grx_forward_light_mgr;

struct grx_fr_light_param_uniform {
    grx_fr_light_param_uniform(grx_shader_program& sp) {
        point_lights_count = sp.get_uniform<int>("point_lights_count");
        spot_lights_count  = sp.get_uniform<int>("spot_lights_count");
        point_lights_idxs  = sp.get_uniform<core::span<int>>("point_lights_idxs");
        spot_lights_idxs   = sp.get_uniform<core::span<int>>("spot_lights_idxs");
        specular_power     = sp.get_uniform<float>("specular_power");
        specular_intensity = sp.get_uniform<float>("specular_intensity");
        eye_pos_ws         = sp.get_uniform<vec3f>("eye_pos_ws");
        dir_light_enabled  = sp.get_uniform<int>("dir_light_enabled");
    }

    grx_fr_light_param_uniform& operator=(const grx_forward_light_mgr&);

    grx_uniform<int>             point_lights_count;
    grx_uniform<int>             spot_lights_count;
    grx_uniform<core::span<int>> point_lights_idxs;
    grx_uniform<core::span<int>> spot_lights_idxs;
    grx_uniform<float>           specular_power;
    grx_uniform<float>           specular_intensity;
    grx_uniform<vec3f>           eye_pos_ws;
    grx_uniform<int>             dir_light_enabled;
};


class grx_mesh_instance;
class grx_mesh_pack;


/**
 * @brief Shadow mapping tech for spot and point lights
 */
class grx_fr_shadow_map_mgr {
public:
    grx_fr_shadow_map_mgr(core::vec2u size);

    /**
     * @brief Shadow path for all lights
     *
     * @param objects - objects for rendering
     * @param shader  - shadow path shader
     * @param spot_light_idxs - indices of spot lights
     * @param spot_lights - spot lights
     */
    void shadow_path(core::span<grx_mesh_instance> objects,
                     const grx_shader_tech&        shader,
                     core::span<const int>         spot_light_idxs,
                     core::span<grx_spot_light>    spot_lights) const;

    /**
     * @brief Shadow path for all lights
     *
     * @param mesh_pack - objects for rendering
     * @param shader  - shadow path shader
     * @param spot_light_idxs - indices of spot lights
     * @param spot_lights - spot lights
     */
    void shadow_path(grx_mesh_pack&             objects,
                     const grx_shader_tech&     shader,
                     core::span<const int>      spot_light_idxs,
                     core::span<grx_spot_light> spot_lights) const;


    /**
     * @brief Bind all textures
     *
     * @param start_id - if for first texture (start_id+1 for second, start_id+2, ...)
     */
    void bind_shadow_map_textures(int start_id);

private:
    core::vec2u       size_;
    uint              fbo_;
    std::vector<uint> spot_maps_ = std::vector<uint>(PE_FORWARD_MAX_SPOT_LIGHTS);
};


class grx_dir_light_provider;
class grx_point_light_provider;
class grx_spot_light_provider;

class grx_forward_light_mgr : public std::enable_shared_from_this<grx_forward_light_mgr> {
public:
    friend class  grx_dir_light_provider;
    friend class  grx_point_light_provider;
    friend class  grx_spot_light_provider;
    //TODO: check this: friend struct core::constructor_accessor<grx_forward_light_mgr>;

    grx_forward_light_mgr(core::constructor_accessor<grx_forward_light_mgr>::cref,
                          core::vec2u csm_shadow_maps_size,
                          core::vec2u spot_shadow_maps_size):
        csm_mgr_          (csm_shadow_maps_size),
        shadow_map_mgr_   (spot_shadow_maps_size),
        free_point_lights_(PE_FORWARD_MAX_POINT_LIGHTS),
        free_spot_lights_ (PE_FORWARD_MAX_SPOT_LIGHTS)
    {
        std::iota(free_point_lights_.begin(), free_point_lights_.end(), 0);
        std::iota(free_spot_lights_.begin(), free_spot_lights_.end(), 0);
    }

    static core::shared_ptr<grx_forward_light_mgr> create_shared(core::vec2u csm_shadow_maps_size,
                                                                 core::vec2u spot_shadow_maps_size) {
        return core::make_shared<grx_forward_light_mgr>(core::constructor_accessor<grx_forward_light_mgr>(),
                csm_shadow_maps_size, spot_shadow_maps_size);
    }

    grx_dir_light_provider   create_dir_light();
    grx_point_light_provider create_point_light();
    grx_spot_light_provider  create_spot_light();

    /**
     * @brief Setup shadow maps to shader
     *
     * @param shader - shader program
     */
    void setup_shadow_maps_to(core::shared_ptr<grx_shader_program>& shader);

    /**
     * @brief Setup shadow maps to shader tech
     *
     * @param tech - shader tech
     */
    void setup_shadow_maps_to(grx_shader_tech& tech);

    /**
     * @brief Shadow path for all lights
     *
     * @param objects - objects for rendering
     * @param shader_program - shader program
     */
    void shadow_path(const core::shared_ptr<grx_camera>& camera,
                     core::span<grx_mesh_instance>       objects,
                     const grx_shader_tech&              shader_tech);

    /**
     * @brief Shadow path for all lights
     *
     * @param mesh_pack - objects for rendering
     * @param shader_program - shader program
     */
    void shadow_path(const core::shared_ptr<grx_camera>& camera,
                     grx_mesh_pack&                      mesh_pack,
                     const grx_shader_tech&              shader_tech);

    /**
     * @brief Setup spot and point light's MVPs
     *
     * @param model - model matrix of object
     * @param shader_program - shader program
     */
    void setup_mvps(const glm::mat4& model, const core::shared_ptr<grx_shader_program>& shader_program) {
        csm_mgr_.setup(shader_program, model);
        for (auto i : spot_lights_idxs_) {
            auto idx = static_cast<size_t>(i);
            cached_uniforms_.find(shader_program.get())->second.
                spot_lights[idx].MVP = spot_lights_[idx].VP * model; // NOLINT
        }
    }

     /**
     * @brief Setup spot and point light's VP's for instanced rendering
     *
     * @param shader_program - shader program
     */
    void setup_vps_instanced(const core::shared_ptr<grx_shader_program>& shader_program) {
        csm_mgr_.setup_instanced(shader_program);
        for (auto i : spot_lights_idxs_) {
            auto idx = static_cast<size_t>(i);
            cached_uniforms_.find(shader_program.get())->second.
                spot_lights[idx].MVP = spot_lights_[idx].VP; // NOLINT
        }
    }

    /**
     * @brief Setup spot and point light's MVPs
     *
     * @param model - model matrix of object
     * @param tech - shader tech
     */
    void setup_mvps(const glm::mat4& model, const grx_shader_tech& tech) {
        setup_mvps(model, tech.base());
        setup_mvps(model, tech.skeleton());
        //setup_mvps(model, tech.instanced());
    }

    /**
     * @brief Setup spot and point light's VP's for instanced rendering
     *
     * @param tech - shader tech
     */
    void setup_vps_instanced(const grx_shader_tech& tech) {
        setup_vps_instanced(tech.instanced());
    }

private:
    void put_back_dir_light() {
        enable_dir_light_ = false;
    }

    void put_back_point_light_num(size_t num) {
        free_point_lights_.push_back(num);
        point_lights_idxs_.erase(
                std::remove(point_lights_idxs_.begin(), point_lights_idxs_.end(), static_cast<int>(num)));
    }

    void put_back_spot_light_num(size_t num) {
        free_spot_lights_.push_back(num);
        spot_lights_idxs_.erase(
                std::remove(spot_lights_idxs_.begin(), spot_lights_idxs_.end(), static_cast<int>(num)));
    }

    struct uniforms_t {
        uniforms_t(core::shared_ptr<grx_shader_program>& shader): param(*shader), dir_light(*shader) {
            for (size_t i = 0; i < PE_FORWARD_MAX_POINT_LIGHTS; ++i)
                point_lights.emplace_back(*shader, i);
            for (size_t i = 0; i < PE_FORWARD_MAX_SPOT_LIGHTS; ++i)
                spot_lights.emplace_back(*shader, i);
        }

        grx_fr_light_param_uniform            param;
        grx_dir_light_uniform                 dir_light;
        core::vector<grx_point_light_uniform> point_lights;
        core::vector<grx_spot_light_uniform>  spot_lights;
    };

    grx_cascade_shadow_map_tech csm_mgr_;
    grx_fr_shadow_map_mgr       shadow_map_mgr_;

    core::hash_map<grx_shader_program*, uniforms_t>           cached_uniforms_;
    grx_dir_light                                             dir_light_;
    core::array<grx_point_light, PE_FORWARD_MAX_POINT_LIGHTS> point_lights_;
    core::array<grx_spot_light, PE_FORWARD_MAX_SPOT_LIGHTS>   spot_lights_;

    std::vector<size_t> free_point_lights_;
    std::vector<size_t> free_spot_lights_;

    std::vector<int>    point_lights_idxs_;
    std::vector<int>    spot_lights_idxs_;

    vec3f eye_position_       = {0.f, 0.f, 0.f};
    float specular_power_     = 0.f;
    float specular_intensity_ = 0.f;
    bool  enable_dir_light_   = false;

public:
    void  specular_power    (float value) { specular_power_ = value; }
    void  specular_intensity(float value) { specular_intensity_ = value; }
    float specular_power    () const { return specular_power_; }
    float specular_intensity() const { return specular_intensity_; }
    vec3f eye_position      () const { return eye_position_; }

    void enable_dir_light (bool value = true) { enable_dir_light_ = value; }
    void disable_dir_light() { enable_dir_light(false); }

    const core::vector<int>& point_lights_idxs() const { return point_lights_idxs_; }
    const core::vector<int>& spot_lights_idxs () const { return spot_lights_idxs_;  }

    bool has_dir_light() const {
        return enable_dir_light_;
    }

    size_t point_lights_count() const {
        return PE_FORWARD_MAX_POINT_LIGHTS - free_point_lights_.size();
    }

    size_t spot_lights_count() const {
        return PE_FORWARD_MAX_SPOT_LIGHTS - free_spot_lights_.size();
    }

    grx_dir_light& dir_light() {
        return dir_light_;
    }

    grx_point_light& point_light(size_t num) {
        return point_lights_.at(num);
    }

    grx_spot_light& spot_light(size_t num) {
        return spot_lights_.at(num);
    }
};


class grx_fr_light_provider_base {
public:
    grx_fr_light_provider_base(core::weak_ptr<grx_forward_light_mgr> mgr): mgr_(core::move(mgr)) {}

    core::shared_ptr<grx_forward_light_mgr> try_mgr() {
        if (auto mgr = mgr_.lock())
            return mgr;
        else
            return nullptr;
    }

    [[nodiscard]]
    core::shared_ptr<const grx_forward_light_mgr> try_mgr() const {
        if (auto mgr = mgr_.lock())
            return mgr;
        else
            return nullptr;
    }

    grx_forward_light_mgr& mgr() {
        if (auto mgr = mgr_.lock())
            return *mgr;
        else
            throw std::runtime_error("Forwad light manager was destroyed!");
    }

    [[nodiscard]]
    const grx_forward_light_mgr& mgr() const {
        if (auto mgr = mgr_.lock())
            return *mgr;
        else
            throw std::runtime_error("Forwad light manager was destroyed!");
    }

protected:
    core::weak_ptr<grx_forward_light_mgr>  mgr_; // NOLINT
};


class grx_dir_light_provider : public grx_fr_light_provider_base {
public:
    grx_dir_light_provider(core::weak_ptr<grx_forward_light_mgr> mgr): grx_fr_light_provider_base(core::move(mgr)) {}

    grx_dir_light_provider(const grx_dir_light_provider&) = delete;
    grx_dir_light_provider& operator=(const grx_dir_light_provider&) = delete;
    grx_dir_light_provider(grx_dir_light_provider&&) noexcept        = default;
    grx_dir_light_provider& operator=(grx_dir_light_provider&&) noexcept = default;

    ~grx_dir_light_provider() {
        if (auto mgr = mgr_.lock())
            mgr->put_back_dir_light();
    }

#define _DEF_GETSET(FIELD_TYPE, FIELD_NAME)                                                                            \
    void FIELD_NAME(FIELD_TYPE value) {                                                                                \
        if (auto mgr = mgr_.lock())                                                                                    \
            mgr->dir_light().FIELD_NAME = value;                                                                       \
        else                                                                                                           \
            throw std::runtime_error("Forward light manager was destroyed!");                                          \
    }                                                                                                                  \
    [[nodiscard]] std::decay_t<FIELD_TYPE> FIELD_NAME() const {                                                        \
        if (auto mgr = mgr_.lock())                                                                                    \
            return mgr->dir_light().FIELD_NAME;                                                                        \
        else                                                                                                           \
            throw std::runtime_error("Forward light manager was destroyed!");                                          \
    }                                                                                                                  \
    [[nodiscard]] core::try_opt<std::decay_t<FIELD_TYPE>> try_##FIELD_NAME() const {                                   \
        if (auto mgr = mgr_.lock())                                                                                    \
            return mgr->dir_light().FIELD_NAME;                                                                        \
        else                                                                                                           \
            return std::runtime_error("Forward light manager was destroyed!");                                         \
    }

    _DEF_GETSET(const vec3f&, color)
    _DEF_GETSET(const vec3f&, direction)
    _DEF_GETSET(float, ambient_intensity)
    _DEF_GETSET(float, diffuse_intensity)

#undef _DEF_GETSET
};

class grx_point_light_provider : public grx_fr_light_provider_base {
public:
    grx_point_light_provider(core::weak_ptr<grx_forward_light_mgr> mgr, size_t num):
        grx_fr_light_provider_base(std::move(mgr)), num_(num) {}

    grx_point_light_provider(const grx_point_light_provider&) = delete;
    grx_point_light_provider& operator=(const grx_point_light_provider&) = delete;
    grx_point_light_provider(grx_point_light_provider&&) noexcept        = default;
    grx_point_light_provider& operator=(grx_point_light_provider&&) noexcept = default;

    ~grx_point_light_provider() {
        if (auto mgr = mgr_.lock())
            mgr->put_back_point_light_num(num_);
    }

#define _DEF_GETSET(FIELD_TYPE, FIELD_NAME)                                                                            \
    void FIELD_NAME(FIELD_TYPE value) {                                                                                \
        if (auto mgr = mgr_.lock())                                                                                    \
            mgr->point_light(num_).FIELD_NAME = value;                                                                 \
        else                                                                                                           \
            throw std::runtime_error("Forward light manager was destroyed!");                                          \
    }                                                                                                                  \
    [[nodiscard]] std::decay_t<FIELD_TYPE> FIELD_NAME() const {                                                        \
        if (auto mgr = mgr_.lock())                                                                                    \
            return mgr->point_light(num_).FIELD_NAME;                                                                  \
        else                                                                                                           \
            throw std::runtime_error("Forward light manager was destroyed!");                                          \
    }                                                                                                                  \
    [[nodiscard]] core::try_opt<std::decay_t<FIELD_TYPE>> try_##FIELD_NAME() const {                                   \
        if (auto mgr = mgr_.lock())                                                                                    \
            return mgr->point_light(num_).FIELD_NAME;                                                                  \
        else                                                                                                           \
            return std::runtime_error("Forward light manager was destroyed!");                                         \
    }

    _DEF_GETSET(const vec3f&, color)
    _DEF_GETSET(const vec3f&, position)
    _DEF_GETSET(float, ambient_intensity)
    _DEF_GETSET(float, diffuse_intensity)
    _DEF_GETSET(float, attenuation_constant)
    _DEF_GETSET(float, attenuation_linear)
    _DEF_GETSET(float, attenuation_quadratic)

#undef _DEF_GETSET

private:
    size_t num_;
};

class grx_spot_light_provider : public grx_fr_light_provider_base {
public:
    grx_spot_light_provider(core::weak_ptr<grx_forward_light_mgr> mgr, size_t num):
        grx_fr_light_provider_base(std::move(mgr)), num_(num) {}

    grx_spot_light_provider(const grx_spot_light_provider&) = delete;
    grx_spot_light_provider& operator=(const grx_spot_light_provider&) = delete;
    grx_spot_light_provider(grx_spot_light_provider&&) noexcept        = default;
    grx_spot_light_provider& operator=(grx_spot_light_provider&&) noexcept = default;

    ~grx_spot_light_provider() {
        if (auto mgr = mgr_.lock())
            mgr->put_back_spot_light_num(num_);
    }

#define _DEF_GETSET(FIELD_TYPE, FIELD_NAME)                                                                            \
    void FIELD_NAME(FIELD_TYPE value) {                                                                                \
        if (auto mgr = mgr_.lock())                                                                                    \
            mgr->spot_light(num_).FIELD_NAME = value;                                                                  \
        else                                                                                                           \
            throw std::runtime_error("Forward light manager was destroyed!");                                          \
    }                                                                                                                  \
    [[nodiscard]] std::decay_t<FIELD_TYPE> FIELD_NAME() const {                                                        \
        if (auto mgr = mgr_.lock())                                                                                    \
            return mgr->spot_light(num_).FIELD_NAME;                                                                   \
        else                                                                                                           \
            throw std::runtime_error("Forward light manager was destroyed!");                                          \
    }                                                                                                                  \
    [[nodiscard]] core::try_opt<std::decay_t<FIELD_TYPE>> try_##FIELD_NAME() const {                                   \
        if (auto mgr = mgr_.lock())                                                                                    \
            return mgr->spot_light(num_).FIELD_NAME;                                                                   \
        else                                                                                                           \
            return std::runtime_error("Forward light manager was destroyed!");                                         \
    }

    _DEF_GETSET(const vec3f&, color)
    _DEF_GETSET(float, ambient_intensity)
    _DEF_GETSET(float, diffuse_intensity)
    _DEF_GETSET(float, attenuation_constant)
    _DEF_GETSET(float, attenuation_linear)
    _DEF_GETSET(float, attenuation_quadratic)
    _DEF_GETSET(float, cutoff)

    void position(const vec3f& value) {
        if (auto mgr = mgr_.lock()) {
            mgr->spot_light(num_).position = value;
            mgr->spot_light(num_).update_view_projection();
        } else
            throw std::runtime_error("Forward light manager was destroyed!");
    }

    [[nodiscard]]
    vec3f position() const {
        if (auto mgr = mgr_.lock())
            return mgr->spot_light(num_).position;
        else
            throw std::runtime_error("Forward light manager was destroyed!");
    }

    [[nodiscard]]
    core::try_opt<vec3f> try_position() const {
        if (auto mgr = mgr_.lock())
            return mgr->spot_light(num_).position;
        else
            throw std::runtime_error("Forward light manager was destroyed!");
    }

    void direction(const vec3f& value) {
        if (auto mgr = mgr_.lock()) {
            mgr->spot_light(num_).direction = value;
            mgr->spot_light(num_).update_view_projection();
        } else
            throw std::runtime_error("Forward light manager was destroyed!");
    }

    [[nodiscard]]
    vec3f direction() const {
        if (auto mgr = mgr_.lock())
            return mgr->spot_light(num_).direction;
        else
            throw std::runtime_error("Forward light manager was destroyed!");
    }

    [[nodiscard]]
    core::try_opt<vec3f> try_direction() const {
        if (auto mgr = mgr_.lock())
            return mgr->spot_light(num_).direction;
        else
            return std::runtime_error("Forward light manager was destroyed!");
    }

#undef _DEF_GETSET

private:
    size_t num_;
};

} // namespace grx

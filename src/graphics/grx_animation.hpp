#pragma once

#include "core/math.hpp"
#include "core/serialization.hpp"
#include "grx_types.hpp"

class aiScene;
class aiAnimation;

namespace grx
{
struct grx_anim_channel_info {
    core::u32 animation_id;
    core::u32 node_id;
};
}

namespace std
{
template <>
struct hash<grx::grx_anim_channel_info> {
    size_t operator()(const grx::grx_anim_channel_info& info) const noexcept {
        static_assert(sizeof(size_t) == 8 || sizeof(size_t) == 4);

        if constexpr (sizeof(size_t) == 8) {
            core::u64 result = info.animation_id;
            result           = result << 32;
            result |= info.node_id;
            return result;
        }
        else {
            core::u32 result = static_cast<core::u16>(info.animation_id);
            result           = result << 16;
            result |= static_cast<core::u16>(info.node_id);
            return result;
        }
    }
};
} // namespace std

namespace grx {
/**
 * @brief Position animation key data
 */
struct grx_position_key {
    double time;
    vec3f  value;
};

/**
 * @brief Scaling animation key data
 */
struct grx_scaling_key {
    double time;
    vec3f  value;
};

/**
 * @brief Rotation animation key data
 */
struct grx_rotation_key {
    double    time;
    glm::quat value;
};

/**
 * @brief Position, Scaling and Rotation key data
 */
struct grx_combined_key {
    PE_SERIALIZE(time, value)

    struct combined_key_value {
        PE_SERIALIZE(position, scaling, rotation)

        vec3f     position;
        vec3f     scaling;
        glm::quat rotation;
    };

    double time;
    combined_key_value value;
};


template <typename T>
concept AnimationKey = requires (T v) {
    {core::move(v.time)} -> std::same_as<double&&>;
    {v.value};
};

template <typename T>
concept AnimationKeyVector = AnimationKey<typename T::value_type> &&
    std::same_as<core::vector<typename T::value_type>, std::remove_const_t<std::remove_reference_t<T>>>;

/**
 * @brief Stores data specific for the animation channel
 */
struct grx_animation_channel {
    core::vector<grx_position_key> position_keys;
    core::vector<grx_scaling_key>  scaling_keys;
    core::vector<grx_rotation_key> rotation_keys;
};

/**
 * @brief Stores animation specific data
 */
class grx_animation { // NOLINT
public:
    grx_animation() = default;
    grx_animation(core::hash_map<core::string, grx_animation_channel> channels,
                  double                                              duration,
                  double                                              ticks_per_second):
        _channels(core::move(channels)), _duration(duration), _ticks_per_second(ticks_per_second) {}

    static grx_animation from_assimp(const aiAnimation* assimpAnimation);

    [[nodiscard]]
    class grx_animation_optimized get_optimized(const class grx_skeleton& skeleton) const;

private:
    core::hash_map<core::string, grx_animation_channel> _channels;
    double _duration;
    double _ticks_per_second;

public:
    [[nodiscard]]
    const auto& channels() const {
        return _channels;
    }

    [[nodiscard]]
    double duration() const {
        return _duration;
    }

    [[nodiscard]]
    double ticks_per_second() const {
        return _ticks_per_second;
    }

    void duration(double value) {
        _duration = value;
    }

    void ticks_per_second(double value) {
        _ticks_per_second = value;
    }
};

/**
 * @brief Stores animation specific data
 */
class grx_animation_optimized { // NOLINT
public:
    PE_SERIALIZE(_channels, _duration, _ticks_per_second)

    grx_animation_optimized() = default;
    grx_animation_optimized(const grx_animation& animation, const class grx_skeleton& skeleton);

private:
    core::vector<core::vector<grx_combined_key>> _channels;
    double                                       _duration;
    double                                       _ticks_per_second;

public:
    [[nodiscard]]
    const auto& channels() const {
        return _channels;
    }

    [[nodiscard]]
    double duration() const {
        return _duration;
    }

    [[nodiscard]]
    double ticks_per_second() const {
        return _ticks_per_second;
    }

    void duration(double value) {
        _duration = value;
    }

    void ticks_per_second(double value) {
        _ticks_per_second = value;
    }
};


/*
 * @brief Viewer for fast animation key lookup by time
 *
 * Uses heuristic algorithm that relies on linear timestep
 * between keys.
 * I.e. it has O(1) for normalized animations where every
 * frame is a keyframe.
 *
 */
template <AnimationKeyVector V>
class grx_animation_key_lookup {
public:
    grx_animation_key_lookup(V& keys): _keys(&keys) {
        Expects(!keys.empty());

        auto count    = _keys->size();
        auto end_time = _keys->back().time;

        _timefactor = static_cast<double>(count) / end_time;
    }

private:
    [[nodiscard]]
    size_t p_index_lookup(double time) const {
        Expects(time >= 0.0);

        auto adjusted_time = time * _timefactor;
        auto approx_index  = static_cast<size_t>(adjusted_time);
        auto approx_sindex = static_cast<ptrdiff_t>(approx_index);

        assert(approx_index < _keys->size());

        size_t idx = 0;
        if (time > (*_keys)[approx_index].time) {
            /* Search forward */
            auto found = std::find_if(_keys->begin() + approx_sindex, _keys->end(), [=](auto& k) {
                return time < k.time;
            });
            assert(found != _keys->end());

            /* Get previous frame */
            found = (found == _keys->begin()) ? _keys->end() - 1 : found - 1;

            idx = static_cast<size_t>(found - _keys->begin());
        }
        else {
            /* Search backward */
            auto found =
                std::find_if(_keys->rbegin() + static_cast<ptrdiff_t>(_keys->size() - approx_index),
                             _keys->rend(),
                             [=](auto& k) { return time >= k.time; });

            if (found == _keys->rend())
                found = _keys->rbegin();

            idx = _keys->size() - static_cast<size_t>(found - _keys->rbegin()) - 1;
        }

        //core::printline("Predicted: {}  Actual: {}  Count: {}", approx_index, idx, _keys->size());

        assert(idx < _keys->size());
        return idx;
    }

public:
    [[nodiscard]]
    size_t index(double time) const {
        return p_index_lookup(fmod(time, _keys->back().time));
    }

    template <typename T>
    struct interstep_t {
        T current;
        T next;
        float interstep_factor;

        template <typename U = T>
        auto interpolate() const -> std::enable_if_t<std::is_same_v<U, vec3f>, U> {
            return lerp(current, next, interstep_factor);
        }

        template <typename U = T>
        auto interpolate() const -> std::enable_if_t<std::is_same_v<U, glm::quat>, U> {
            return normalize(slerp(current, next, interstep_factor));
        }
        template <typename U = T>
        auto interpolate() const -> std::enable_if_t<std::is_same_v<T, grx_combined_key::combined_key_value>, U> {
            return grx_combined_key::combined_key_value{
                lerp(current.position, next.position, interstep_factor),
                lerp(current.scaling, next.scaling, interstep_factor),
                normalize(slerp(current.rotation, next.rotation, interstep_factor)),
            };
        }
    };

    [[nodiscard]]
    auto interstep(double time) {
        time = fmod(time, _keys->back().time);

        auto idx  = p_index_lookup(time);
        auto cur  = _keys->begin() + static_cast<ptrdiff_t>(idx);
        auto next = (idx + 1 == _keys->size()) ? _keys->begin() : cur + 1;
        auto factor = static_cast<float>((time - cur->time) / (next->time - cur->time));

        return interstep_t<std::remove_reference_t<decltype(cur->value)>>{
            cur->value, next->value, factor};
    }

    [[nodiscard]]
    decltype(auto) key(double time) {
        return (*_keys)[index(time)];
    }

    [[nodiscard]]
    decltype(auto) operator[](double time) {
        return key(time);
    }

private:
    V*     _keys;
    double _timefactor;
};

core::hash_map<core::string, grx_animation> get_animations_from_assimp(const aiScene* scene);
core::hash_map<core::string, grx_animation_optimized>
get_animations_optimized_from_assimp(const aiScene* scene, const grx_skeleton& skeleton);

} // namespace grx


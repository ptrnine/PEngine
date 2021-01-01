#include "grx_animation.hpp"
#include "core/math.hpp"
#include "grx_skeleton.hpp"
#include <assimp/anim.h>
#include <assimp/scene.h>

using namespace core;

namespace
{
inline bool less(double a, double b) {
    return definitely_less(a, b, 0.0000001);
}
inline bool eq(double a, double b) {
    return approx_equal(a, b, 0.0000001);
}
} // namespace

class aiScene;

namespace grx {

struct combined_key_opt {
    double              time;
    optional<vec3f>     pos;
    optional<vec3f>     scale;
    optional<glm::quat> rot;

    void clear() {
        pos.reset();
        scale.reset();
        rot.reset();
    }

    [[nodiscard]]
    bool empty() const {
        return !pos && !scale && !rot;
    }

    void update_pos(const optional<pair<double, vec3f>>& value) {
        if (value) {
            time = value->first;
            pos  = value->second;
        }
    }

    void update_scale(const optional<pair<double, vec3f>>& value) {
        if (value) {
            if (empty() || eq(value->first, time)) {
                time  = value->first;
                scale = value->second;
            }
            else if (less(value->first, time)) {
                clear();
                time  = value->first;
                scale = value->second;
            }
        }
    }

    void update_rot(const optional<pair<double, glm::quat>>& value) {
        if (value) {
            if (empty() || eq(value->first, time)) {
                time = value->first;
                rot  = value->second;
            }
            else if (less(value->first, time)) {
                clear();
                time = value->first;
                rot  = value->second;
            }
        }
    }
};

vector<grx_combined_key>
combine_animation_keys(const vector<grx_position_key>& position_keys,
                     const vector<grx_scaling_key>&  scaling_keys,
                     const vector<grx_rotation_key>& rotation_keys) {
    //printline("pos: {} scale: {} rot: {}", position_keys.size(), scaling_keys.size(), rotation_keys.size());

    Expects(!position_keys.empty());
    Expects(!scaling_keys.empty());
    Expects(!rotation_keys.empty());

    auto max_size = std::max({position_keys.size(), scaling_keys.size(), rotation_keys.size()});

    vector<combined_key_opt> combined_keys;
    combined_keys.reserve(max_size);

    auto p = position_keys.begin();
    auto s = scaling_keys.begin();
    auto r = rotation_keys.begin();

    auto pe = position_keys.end();
    auto se = scaling_keys.end();
    auto re = rotation_keys.end();

    while (p != pe || s != se || r != re) {
        optional<pair<double, vec3f>>     newpos;
        optional<pair<double, vec3f>>     newscale;
        optional<pair<double, glm::quat>> newrot;

        if (p != pe)
            newpos.emplace(p->time, p->value);
        if (s != se)
            newscale.emplace(s->time, s->value);
        if (r != re)
            newrot.emplace(r->time, r->value);

        combined_key_opt newkey;
        newkey.update_pos(newpos);
        newkey.update_scale(newscale);
        newkey.update_rot(newrot);

        combined_keys.emplace_back(newkey);

        if (newkey.pos) ++p;
        if (newkey.scale) ++s;
        if (newkey.rot) ++r;
    }

    auto end = combined_keys.end();

    auto start_pos   = std::find_if(combined_keys.begin(), end, xlambda(c, c.pos.has_value()));
    auto start_scale = std::find_if(combined_keys.begin(), end, xlambda(c, c.scale.has_value()));
    auto start_rot   = std::find_if(combined_keys.begin(), end, xlambda(c, c.rot.has_value()));

    auto end_pos   = end;
    auto end_scale = end;
    auto end_rot   = end;

    for (auto i = combined_keys.begin(); i != end; ++i) {
        if (i->pos) {
            start_pos = i;
            end_pos = std::find_if(start_pos + 1, end, xlambda(c, c.pos.has_value()));
        } else {
            if (end_pos == end)
                i->pos = start_pos->pos;
            else {
                auto f = inverse_lerp(start_pos->time, end_pos->time, i->time);
                i->pos = lerp(*start_pos->pos, *end_pos->pos, static_cast<float>(f));
            }
        }
        if (i->scale) {
            start_scale = i;
            end_scale = std::find_if(start_scale + 1, end, xlambda(c, c.scale.has_value()));
        } else {
            if (end_scale == end)
                i->scale = start_scale->scale;
            else {
                auto f = inverse_lerp(start_scale->time, end_scale->time, i->time);
                i->scale = lerp(*start_scale->scale, *end_scale->scale, static_cast<float>(f));
            }
        }
        if (i->rot) {
            start_rot = i;
            end_rot = std::find_if(start_rot + 1, end, xlambda(c, c.rot.has_value()));
        } else {
            if (end_rot == end)
                i->rot = start_rot->rot;
            else {
                auto f = inverse_lerp(start_rot->time, end_rot->time, i->time);
                i->rot = normalize(slerp(*start_rot->rot, *end_rot->rot, static_cast<float>(f)));
            }
        }
    }

    auto result = vector<grx_combined_key>(combined_keys.size());
    for (auto& [dst, src] : zip_view(result, combined_keys)) {
        dst.time = src.time;
        dst.value.position = src.pos.value();
        dst.value.scaling  = src.scale.value();
        dst.value.rotation = src.rot.value();
    }
    //printline("result: {}", result.size());

    return result;
}

grx_animation grx_animation::from_assimp(const aiAnimation* animation) {
    core::hash_map<core::string, grx_animation_channel> channels;

    double duration = animation->mDuration;
    double ticks_per_second = animation->mTicksPerSecond;

    for (auto channel : span(animation->mChannels, animation->mNumChannels)) {
        auto [pos, was_inserted] =
            channels.emplace(string(channel->mNodeName.data), grx_animation_channel());

        if (was_inserted) {
            auto& src_channel = pos->second;

            src_channel.position_keys.resize(channel->mNumPositionKeys);
            src_channel.scaling_keys.resize(channel->mNumScalingKeys);
            src_channel.rotation_keys.resize(channel->mNumRotationKeys);

            auto src_position_keys = span(channel->mPositionKeys, channel->mNumPositionKeys);
            auto src_scaling_keys  = span(channel->mScalingKeys, channel->mNumScalingKeys);
            auto src_rotation_keys = span(channel->mRotationKeys, channel->mNumRotationKeys);

            for (auto& [dst, src] : zip_view(src_channel.position_keys, src_position_keys)) {
                dst.time  = src.mTime;
                dst.value = vec{src.mValue.x, src.mValue.y, src.mValue.z};
            }
            for (auto& [dst, src] : zip_view(src_channel.scaling_keys, src_scaling_keys)) {
                dst.time  = src.mTime;
                dst.value = vec{src.mValue.x, src.mValue.y, src.mValue.z};
            }
            for (auto& [dst, src] : zip_view(src_channel.rotation_keys, src_rotation_keys)) {
                dst.time  = src.mTime;
                dst.value = glm::quat(src.mValue.w, src.mValue.x, src.mValue.y, src.mValue.z);
            }
        }
    }

    return grx_animation(channels, duration, ticks_per_second);
}

grx_animation_optimized grx_animation::get_optimized(const grx_skeleton& skeleton) const {
    return grx_animation_optimized(*this, skeleton);
}

grx_animation_optimized::grx_animation_optimized(const grx_animation& animation,
                                                 const grx_skeleton&  skeleton):
    _duration(animation.duration()), _ticks_per_second(animation.ticks_per_second()) {
    _channels.resize(animation.channels().size());

    for (auto& [name, channel_data] : animation.channels()) {
        /*
         * TODO: skipping anim bones may influence for final transformation
         * need check this
         */
        auto idx_found = skeleton.skeleton_data().mapping.find(name);
        if (idx_found != skeleton.skeleton_data().mapping.end())
            _channels[idx_found->second] = combine_animation_keys(
                channel_data.position_keys, channel_data.scaling_keys, channel_data.rotation_keys);
    }
}

hash_map<string, grx_animation> get_animations_from_assimp(const aiScene* scene) {
    if (!scene->HasAnimations())
        throw std::runtime_error("Scene does not have animations");

    hash_map<string, grx_animation> animations;

    for (auto anim : span(scene->mAnimations, scene->mNumAnimations))
        animations.emplace(core::string(anim->mName.C_Str(), anim->mName.length),
                           grx_animation::from_assimp(anim));

    return animations;
}

hash_map<string, grx_animation_optimized>
get_animations_optimized_from_assimp(const aiScene* scene, const grx_skeleton& skeleton) {
    if (!scene->HasAnimations())
        throw std::runtime_error("Scene does not have animations");

    hash_map<string, grx_animation_optimized> animations;

    for (auto anim : span(scene->mAnimations, scene->mNumAnimations))
        animations.emplace(core::string(anim->mName.C_Str(), anim->mName.length),
                           grx_animation::from_assimp(anim).get_optimized(skeleton));

    return animations;
}

} // namespace grx

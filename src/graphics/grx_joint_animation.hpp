#pragma once

#include <glm/gtc/quaternion.hpp>
#include "core/config_manager.hpp"
#include "core/helper_macros.hpp"
#include "core/math.hpp"
#include "core/types.hpp"
#include "core/vec.hpp"

namespace grx {

    enum class joint_anim_interpolation_t {
        linear = 0,
        quadratic,
        square
    };

    struct grx_joint_anim_frame {
        core::u32                  frame_number;
        core::vec3f                position;
        core::vec3f                degree_rotation;
        joint_anim_interpolation_t interpolation_type;

        TUPLE_SERIALIZE(frame_number, position, degree_rotation, interpolation_type)
    };

    struct grx_joint_anim {
        grx_joint_anim(const core::config_manager& config_mgr, const core::string& section) {
            frames   = config_mgr.read<core::vector<grx_joint_anim_frame>>(section, "frames");
            repeat   = config_mgr.read_default(section, "repeat", false);
            repeats  = config_mgr.read_default<core::u32>(section, "repeats", 1);
            duration = config_mgr.read<double>(section, "duration");
        }

        core::vector<grx_joint_anim_frame> frames;
        bool repeat;
        core::u32 repeats;
        double duration;
    };

    struct grx_joint_animation_holder {
        grx_joint_animation_holder() = default;
        grx_joint_animation_holder(const core::config_manager& config_mgr) {
            for (auto& [section_name, _] : config_mgr.sections())
                if (section_name.starts_with("joint_anim_"))
                    anims.emplace(section_name.substr(sizeof("joint_anim_") - 1),
                                  grx_joint_anim(config_mgr, section_name));
        }

        [[nodiscard]] core::tuple<core::vec3f, core::vec3f, bool> anim_lookup(
            const core::string& anim_name, double anim_time, core::u32* repeats = nullptr) const {

            auto found_anim = anims.find(anim_name);
            if (found_anim == anims.end()) {
                LOG_WARNING("Joint animation {} was not found!", anim_name);
                return {core::vec3f::filled_with(0.f), core::vec3f::filled_with(0.f), true};
            }

            auto& anim = found_anim->second;
            bool wrapped = anim_time >= anim.duration;
            if (repeats) {
                ++(*repeats);
                if (*repeats < anim.repeats)
                    wrapped = false;
            }

            anim_time = std::fmod(anim_time, anim.duration);

            auto last_frame        = anim.frames.back().frame_number;
            auto frame_per_second  = static_cast<double>(last_frame) / anim.duration;
            auto current_frame     = anim_time * frame_per_second;
            auto current_frame_int = static_cast<core::u32>(current_frame);

            auto end_frame =
                anim.frames / core::find_if([&](const grx_joint_anim_frame& f) {
                    return current_frame_int < f.frame_number;
                });
            auto start_frame = end_frame - 1;

            double factor = core::inverse_lerp(
                double(start_frame->frame_number), double(end_frame->frame_number), current_frame);

            switch (start_frame->interpolation_type) {
                case joint_anim_interpolation_t::linear:
                    break;
                case joint_anim_interpolation_t::quadratic:
                    factor *= factor;
                    break;
                case joint_anim_interpolation_t::square:
                    factor = std::sqrt(factor);
                    break;
            }

            return {
                core::lerp(start_frame->position, end_frame->position, float(factor)),
                core::lerp(start_frame->degree_rotation, end_frame->degree_rotation, float(factor)),
                wrapped
            };
        }

        core::hash_map<core::string, grx_joint_anim> anims;
    };

    class grx_joint_animation_player {
    public:
        struct anim_spec {
            core::timer timer;
            core::timer suspend_timer;
            core::u32   repeats = 0;
            bool        on_suspend = false;
        };

        void play_animation(const core::string& animation) {
            _anims.insert_or_assign(animation, anim_spec{});
        }

        bool suspend_animation(const core::string& animation) {
            auto found_anim = _anims.find(animation);
            if (found_anim == _anims.end())
                return false;

            found_anim->second.on_suspend = true;
            found_anim->second.suspend_timer.reset();
            return true;
        }

        bool resume_animation(const core::string& animation) {
            auto found_anim = _anims.find(animation);
            if (found_anim == _anims.end())
                return false;

            if (!found_anim->second.on_suspend)
                return false;

            found_anim->second.on_suspend = false;
            found_anim->second.timer.add_to_timepoint(found_anim->second.suspend_timer.tick());
            return true;
        }

        void update(grx_joint_animation_holder& holder) {
            _position.set(0.f, 0.f, 0.f);
            _rotation.set(0.f, 0.f, 0.f);

            std::vector<core::string_view> to_delete;
            for (auto& [anim, spec] : _anims) {
                auto [pos, rot, end] =
                    holder.anim_lookup(anim,
                                       spec.on_suspend ? spec.timer.measure_count() -
                                                             spec.suspend_timer.measure_count()
                                                       : spec.timer.measure_count(),
                                       &spec.repeats);
                if (end)
                    to_delete.push_back(anim);
                else {
                    _position += pos;
                    _rotation += rot;
                }
            }
            for (auto& anim : to_delete)
                _anims.erase(core::string(anim));
        }

        [[nodiscard]]
        const core::vec3f& position() const {
            return _position;
        }

        [[nodiscard]]
        const core::vec3f& rotation() const {
            return _rotation;
        }

    private:
        core::hash_map<core::string, anim_spec> _anims;
        core::vec3f                             _position = {0.f, 0.f, 0.f};
        core::vec3f                             _rotation = {0.f, 0.f, 0.f};
    };
}

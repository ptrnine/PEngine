#include "grx_types.hpp"
#include "grx_skeleton.hpp"
#include "grx_animation.hpp"

namespace grx
{
enum class grx_anim_permit {
    displace = 0,
    replace,
    suspend,
    simultaneously
};

class grx_anim_params;
class grx_animation_player;

class grx_anim_hook {
public:
    grx_anim_hook() = default;

    template <typename Function>
    grx_anim_hook(double istart_factor, Function&& icallback):
        start_factor(istart_factor), callback(core::move(icallback)) {}

    template <typename Function>
    grx_anim_hook(Function&& icallback): callback(core::move(icallback)) {}

    void update(grx_animation_player& player, const grx_anim_params& params, double progress_factor) {
        if (callback && core::definitely_less(start_factor, progress_factor, 0.00001)) {
            callback(player, params);
            callback = nullptr;
        }
    }

private:
    double start_factor = 0.0;
    core::function<void(grx_animation_player&, const grx_anim_params&)> callback;
};

using grx_anim_callback = core::function<void(grx_animation_player&, const grx_anim_params&)>;

class grx_anim_params {
public:
    grx_anim_params() = default;
    grx_anim_params(core::string iname): name(core::move(iname)) {}
    grx_anim_params(core::string    iname,
                    double          iduration                    = 1.0,
                    bool            istop_at_end                 = true,
                    grx_anim_permit ipermit                      = grx_anim_permit::replace,
                    double          isuspend_transition_duration = 0.0,
                    double          iresume_transition_duration  = 0.0):
        name(core::move(iname)),
        speed_factor(1.0 / iduration),
        suspend_transition_duration(isuspend_transition_duration),
        resume_transition_duration(iresume_transition_duration),
        permit(ipermit),
        stop_at_end(istop_at_end) {}

    template <core::Duration D1 = core::seconds,
              core::Duration D2 = core::seconds,
              core::Duration D3 = core::seconds>
    grx_anim_params(core::string    iname,
                    D1              iduration                    = core::seconds(1),
                    bool            istop_at_end                 = true,
                    grx_anim_permit ipermit                      = grx_anim_permit::replace,
                    D2              isuspend_transition_duration = core::seconds(0),
                    D3              iresume_transition_duration  = core::seconds(0)):
        name(core::move(iname)),
        speed_factor(1.0 / core::duration_to_float_seconds(iduration)),
        suspend_transition_duration(core::duration_to_float_seconds(isuspend_transition_duration)),
        resume_transition_duration(core::duration_to_float_seconds(iresume_transition_duration)),
        permit(ipermit),
        stop_at_end(istop_at_end) {}

    core::string    name;
    double          speed_factor                = 1.0;
    double          suspend_transition_duration = 0.0;
    double          resume_transition_duration  = 0.0;
    grx_anim_permit permit                      = grx_anim_permit::replace;
    bool            stop_at_end                 = true;
};

class grx_animation_player {
public:
    struct anim_spec_t {
        double            progress = 0.0;
        grx_anim_params   params;
        grx_anim_hook     hook;
        grx_anim_callback on_suspend = {};
        grx_anim_callback on_resume = {};
    };

    struct transition_t {
        core::string start_anim_name;
        double       start_anim_factor;
        core::string end_anim_name;
        double       end_anim_factor;
        double       cur = 0.0;
        double       end;
    };

    grx_anim_params* current_animation_params() {
        if (!_anims.empty())
            return &_anims.back().params;
        else
            return nullptr;
    }

    [[nodiscard]]
    const anim_spec_t* current_animation() const {
        if (!_anims.empty())
            return &_anims.back();
        else
            return nullptr;
    }

    void play_animation(grx_anim_params params) {
        if (!_anims.empty() && _anims.back().params.permit == grx_anim_permit::replace)
            _anims.back() = anim_spec_t{0.0, core::move(params), {}};
        else {
            setup_suspend_transition(params);
            _anims.emplace_back(anim_spec_t{0.0, core::move(params), {}});
        }
    }

    void play_animation(grx_anim_params   params,
                        grx_anim_hook     hook,
                        grx_anim_callback on_suspend = {},
                        grx_anim_callback on_resume  = {}) {
        if (!_anims.empty() && _anims.back().params.permit == grx_anim_permit::replace)
            _anims.back() = anim_spec_t{0.0,
                                        core::move(params),
                                        core::move(hook),
                                        core::move(on_suspend),
                                        core::move(on_resume)};
        else {
            setup_suspend_transition(params);
            if (!_anims.empty() && _anims.back().on_suspend)
                _anims.back().on_suspend(*this, _anims.back().params);
            _anims.emplace_back(anim_spec_t{0.0,
                                            core::move(params),
                                            core::move(hook),
                                            core::move(on_suspend),
                                            core::move(on_resume)});
        }
    }

    [[nodiscard]]
    auto& animations() const {
        return _anims;
    }

    template <typename function>
    void foreach_params(function&& callback) {
        for (auto& spec : _anims)
            if (callback(spec.params))
                break;
    }

    template <typename function>
    void foreach_params_back(function&& callback) {
        for (auto i = _anims.rbegin(); i != _anims.rend(); ++i)
            if (callback(i->params))
                break;
    }

    template <typename function>
    void foreach_params(function&& callback) const {
        for (auto& spec : _anims)
            if (callback(spec.params))
                break;
    }

    template <typename function>
    void foreach_params_back(function&& callback) const {
        for (auto i = _anims.rbegin(); i != _anims.rend(); ++i)
            if (callback(i->params))
                break;
    }

    template <typename function>
    void foreach(function&& callback) const {
        for (auto& spec : _anims)
            if (callback(spec))
                break;
    }

    template <typename function>
    void foreach_back(function&& callback) const {
        for (auto i = _anims.rbegin(); i != _anims.rend(); ++i)
            if (callback(*i))
                break;
    }

    template <typename function>
    void foreach(function&& callback) {
        for (auto& spec : _anims)
            if (callback(spec))
                break;
    }

    template <typename function>
    void foreach_back(function&& callback) {
        for (auto i = _anims.rbegin(); i != _anims.rend(); ++i)
            if (callback(*i))
                break;
    }

    [[nodiscard]]
    const core::vector<glm::mat4>& final_transforms() const {
        return _final_transforms;
    }

protected:
    void setup_suspend_transition(const grx_anim_params& params) {
        constexpr double zero = 0.0;
        if (!_anims.empty() &&
            std::memcmp(&_anims.back().params.suspend_transition_duration, &zero, sizeof(double)) !=
                0 &&
            (_anims.back().params.permit == grx_anim_permit::suspend ||
             _anims.back().params.permit == grx_anim_permit::displace)) {
            _active_transition = transition_t{_anims.back().params.name,
                                              _anims.back().progress,
                                              params.name,
                                              0.0,
                                              0.0001,
                                              _anims.back().params.suspend_transition_duration};
        }
    }

    bool setup_resume_transition(const grx_anim_params& params) {
        constexpr double zero = 0.0;
        if (!_anims.empty() &&
            std::memcmp(&_anims.back().params.resume_transition_duration, &zero, sizeof(double)) !=
                0 &&
            (_anims.back().params.permit == grx_anim_permit::suspend ||
             _anims.back().params.permit == grx_anim_permit::displace)) {
            _active_transition = transition_t{params.name,
                                              0.9999,
                                              _anims.back().params.name,
                                              _anims.back().progress,
                                              0.0001,
                                              _anims.back().params.resume_transition_duration};
            return true;
        }
        return false;
    }

    bool transition_update(core::hash_map<core::string, grx_animation_optimized>* animations,
                           grx_skeleton_optimized*                                skeleton,
                           double                                                 framestep) {
        if (_active_transition) {
            auto& t = *_active_transition;

            if (t.cur > t.end) {
                _active_transition.reset();
                return false;
            }

            if (animations) {
                auto factor = t.cur / t.end;

                auto start_anim_pos = animations->find(t.start_anim_name);
                auto end_anim_pos   = animations->find(t.end_anim_name);
                if (start_anim_pos == animations->end() || end_anim_pos == animations->end()) {
                    /* TODO: anim not found */
                    _active_transition.reset();
                    return false;
                }

                auto& start_anim = start_anim_pos->second;
                auto& end_anim   = end_anim_pos->second;

                _final_transforms = skeleton->animation_interpolate_factor_transform(
                    start_anim, end_anim, t.start_anim_factor, t.end_anim_factor, factor);
            }

            t.cur += framestep;
            return true;
        }
        return false;
    }

public:
    void persistent_anim_update(core::hash_map<core::string, grx_animation_optimized>* animations,
                                grx_skeleton_optimized*                                skeleton,
                                double                                                 framestep) {
        if (_anims.empty())
            return;

        if (transition_update(animations, skeleton, framestep))
            return;

        _final_transforms.clear();

        for (auto i = _anims.rbegin(); i != _anims.rend();) {
            auto& spec = *i;

            grx_animation_optimized* anim = nullptr;

            if (animations) {
                auto anim_pos = animations->find(spec.params.name);
                if (anim_pos == animations->end()) {
                    LOG_WARNING("Animation {} was not found", spec.params.name);
                    auto params = core::move(spec.params);
                    auto hook   = core::move(spec.hook);
                    i = decltype(i)(_anims.erase(std::next(i).base()));
                    hook.update(*this, params, 1.0);
                    continue;
                }
                anim = &anim_pos->second;
            }

            if (spec.params.stop_at_end && spec.progress > 1.0) {
                auto params  = core::move(spec.params);
                auto hook    = core::move(spec.hook);
                i = decltype(i)(_anims.erase(std::next(i).base()));
                hook.update(*this, params, 1.0);

                /* Skip previous in this frame if resume success */
                if (setup_resume_transition(params)) {
                    _anims.back().on_resume(*this, _anims.back().params);
                    if (transition_update(animations, skeleton, framestep))
                        return;
                }
                continue;
            }

            switch (spec.params.permit) {
                case grx_anim_permit::simultaneously:
                    if (anim) {
                        auto final_transforms =
                            skeleton->animation_factor_transforms(*anim, spec.progress);
                        if (_final_transforms.empty())
                            _final_transforms = core::move(final_transforms);
                        else {
                            for (auto& [dst, src] : core::zip_view(_final_transforms, final_transforms))
                                dst += src;
                        }
                    }
                    break;
                default:
                    if (anim && _final_transforms.empty())
                        _final_transforms = skeleton->animation_factor_transforms(*anim, spec.progress);
            }

            spec.hook.update(*this, spec.params, spec.progress);

            if (!(spec.params.permit == grx_anim_permit::suspend && i != _anims.rbegin()))
                spec.progress += framestep * spec.params.speed_factor;

            ++i;
        }
    }

private:
    core::list<anim_spec_t>      _anims;
    core::vector<glm::mat4>      _final_transforms;
    core::optional<transition_t> _active_transition;
};

template <bool HasSkeleton>
struct grx_animation_player_if_has_skeleton {
    using type = grx_animation_player;
};

template <>
struct grx_animation_player_if_has_skeleton<false> {
    using type = struct{};
};

} // namespace grx

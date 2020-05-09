#pragma once

#include <core/helper_macros.hpp>
#include <core/types.hpp>
#include <core/vec.hpp>

#include <gainput/gainput.h>

struct GLFWwindow;

namespace inp {
    using inp_input_map_vector = core::vector<core::shared_ptr<gainput::InputMap>>;
    using inp_input_mgr_shared = core::shared_ptr<gainput::InputManager>;


    struct inp_changer_mouse_pos {
        core::optional<core::vec2i> pos;

        void set_and_activate(const core::vec2i& position) {
            pos = position;
        }

        core::optional<core::vec2i> try_get_and_deactivate() {
            if (pos) {
                core::vec2i res = *pos;
                pos = std::nullopt;
                return res;
            } else {
                return std::nullopt;
            }
        }
    };

    struct inp_window_state {
        bool on_focus = false;

        inp_changer_mouse_pos mouse_pos;
    };

    struct inp_state {
        inp_input_mgr_shared mgr;
        inp_input_map_vector maps;
        inp_window_state     state;
    };

    /*
     * Global input context
     */
    class inp_input_ctx {
        SINGLETON_IMPL(inp_input_ctx);

    private:
        inp_input_ctx() = default;

    private:
        inp_state& input_state_for(GLFWwindow* wnd) {
            auto [position, was_inserted] = _window_map.emplace(wnd, inp_state());

            if (was_inserted)
                position->second.mgr = core::make_shared<gainput::InputManager>();

            return position->second;
        }

    public:
        /**
         * Get input manager for grx_window
         *
         * Mapping will be automaticaly destroyed, if manager has no users
         */
        core::shared_ptr<gainput::InputManager> input_mgr_for(GLFWwindow* wnd = nullptr) {
            return input_state_for(wnd).mgr;
        }

        /*
         * Get grx_window states
         */
        inp_window_state& window_state_for(GLFWwindow* wnd = nullptr) {
            return input_state_for(wnd).state;
        }

        /**
         * Create input map for grx_window
         *
         * Returned map available before the grx_window will be destroyed
         */
        core::weak_ptr<gainput::InputMap> create_input_map(GLFWwindow* wnd = nullptr) {
            auto& st = input_state_for(wnd);
            return st.maps.emplace_back(core::make_shared<gainput::InputMap>(*st.mgr));
        }

        /**
         * Global update function
         *
         * Must be called once at the start of game loop
         */
        void update();

    protected:
        core::hash_map<GLFWwindow*, inp_state> _window_map;
    };


    /**
     * Returns global input context
     */
    inline inp_input_ctx& inp_ctx() {
        return inp_input_ctx::instance();
    }

} // namespace inp


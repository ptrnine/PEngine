#pragma once

#include <core/types.hpp>
#include <core/vec.hpp>
#include <core/helper_macros.hpp>
#include <optional>

struct pollfd;

namespace inp
{
enum class mouse_id : core::u32 { mice = 0 };

class mouse_input_service {
public:
    static constexpr size_t MOUSE_DATA_SIZE  = 3;
    static constexpr int    MOUSE_POLL_DELAY = 100;
    static constexpr double MOUSE_POS_FACTOR = 0.0001;

    /* Wrapper for std::atomic, enables copy constructor */
    template <typename T>
    struct atomwrap {
        atomwrap() = default;
        ~atomwrap() = default;
        atomwrap(const atomwrap& ia): a(ia.a.load()) {}
        atomwrap& operator=(const atomwrap<T>& ia) {
            a.store(ia);
        }
        atomwrap(const std::atomic<T>& ia): a(ia.load()) {}

        atomwrap(atomwrap&&) noexcept = default;
        atomwrap& operator=(atomwrap&&) noexcept = default;

        std::atomic<T> a;
    };

    using mouse_pos_vec_t = core::vector<atomwrap<core::u64>>;

    mouse_input_service();
    ~mouse_input_service();

    mouse_input_service(const mouse_input_service&) = delete;
    mouse_input_service& operator=(const mouse_input_service&) = delete;
    mouse_input_service(mouse_input_service&&) = delete;
    mouse_input_service& operator=(mouse_input_service&&) = delete;

    mouse_id add_mouse_device(const core::string& path);

    void update_positions();
    core::vec2f get_last_position(mouse_id id);

private:
    static void mouse_input_service_f(std::atomic<bool>&   awork,
                                     core::vector<pollfd>& mouses,
                                     mouse_pos_vec_t&      positions);

    void start();
    void stop();

    std::thread                 running_thread;
    std::atomic<bool>           work = false;
    core::vector<struct pollfd> mouses;
    mouse_pos_vec_t             positions;
    std::vector<core::vec2f>    last_positions;
};

} // namespace inp

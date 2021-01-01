#include "inp_unix_mouse.hpp"
#include <core/async.hpp>
#include <core/container_extensions.hpp>

extern "C" {
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
}

using namespace core;

namespace inp
{

mouse_input_service::mouse_input_service() {
    add_mouse_device("/dev/input/mice");
}

mouse_input_service::~mouse_input_service() {
    stop();

    for (auto& mouse : mouses)
        close(mouse.fd);
}

void mouse_input_service::mouse_input_service_f(std::atomic<bool>&          awork,
                                                std::vector<struct pollfd>& mouses,
                                                mouse_pos_vec_t&            positions) {
    bool work = awork.load();
    char buff[MOUSE_DATA_SIZE]; // NOLINT
    auto timers = vector<timer>(mouses.size());

    i32 avg_time_ms = 20; // NOLINT

    while (work) {
        work = awork.load();

        int rc = ::poll(mouses.data(), static_cast<nfds_t>(mouses.size()), MOUSE_POLL_DELAY);

        if (rc <= 0)
            continue;

        for (size_t i = 0; i < mouses.size(); ++i) {
            auto& mouse_fd = mouses[i];

            if (mouse_fd.revents) {
                auto readed = read(mouse_fd.fd, buff, MOUSE_DATA_SIZE);
                if (readed != MOUSE_DATA_SIZE)
                    std::cout << "WTF" << std::endl;

                auto pos_raw = positions[i].a.load();
                auto pos     = vec{static_cast<int32_t>(pos_raw >> 32), // NOLINT
                               static_cast<int32_t>(pos_raw & 0x00000000ffffffff)}; // NOLINT

                auto duration = timers[i].tick_count<i32, microseconds::period>();
                if (duration > avg_time_ms * 2) {
                    pos += vec{buff[1], buff[2]} * avg_time_ms;
                } else {
                    pos += vec{buff[1], buff[2]} * duration;
                    avg_time_ms = (avg_time_ms + duration) / 2;
                }

                uint64_t result = static_cast<uint64_t>(static_cast<uint32_t>(pos.x())) << 32; // NOLINT
                result += static_cast<uint32_t>(pos.y());

                positions[i].a = result;
            }
        }

        std::this_thread::yield();
    }
}

void mouse_input_service::start() {
    work = true;
    running_thread = std::thread([&]{
        mouse_input_service_f(work, mouses, positions);
    });
}

void mouse_input_service::stop() {
    work = false;
    running_thread.join();
}

mouse_id mouse_input_service::add_mouse_device(const string& path) {
    if (work)
        stop();

    auto fd = open(path.data(), O_RDONLY | O_NONBLOCK); // NOLINT

    if (fd == -1) {
        start();
        throw std::runtime_error("Can't open mouse device at path " + path);
    }
    positions.emplace_back(0);
    mouses.emplace_back();
    mouses.back().fd = fd;
    mouses.back().events = POLLIN;
    mouses.back().revents = 0;
    last_positions.emplace_back();

    start();

    return static_cast<mouse_id>(mouses.size() - 1);
}

void mouse_input_service::update_positions() {
    for (u32 id = 0; id < static_cast<u32>(mouses.size()); ++id) {
        uint64_t pos = positions.at(id).a.exchange(0);
        auto x = static_cast<double>(static_cast<int32_t>(pos >> 32)); // NOLINT
        auto y = static_cast<double>(static_cast<int32_t>(pos & 0x00000000ffffffff)); // NOLINT
        x *= MOUSE_POS_FACTOR;
        y *= MOUSE_POS_FACTOR;

        auto mov = static_cast<vec2f>(vec{x, -y});
        last_positions.at(id) = mov;
    }
}

vec2f mouse_input_service::get_last_position(mouse_id id) {
    auto raw_id = static_cast<u32>(id);
    return last_positions.at(raw_id);
}

} // namespace inp

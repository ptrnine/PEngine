#include "inp_linux.hpp"
#include "core/files.hpp"
#include "core/log.hpp"
#include "core/container_extensions.hpp"
#include "core/time.hpp"
#include "core/views/split.hpp"
#include "core/acts/to.hpp"
#include <fcntl.h>
#include <filesystem>
#include <linux/input.h>
#include <sys/poll.h>

//using namespace core;
namespace fs = std::filesystem;

#define DEVICES_INFO_PATH "/proc/bus/input/devices"
constexpr core::string_view NAME_PREFIX = "N: Name=\"";
constexpr core::string_view HANDLERS_PREFIX = "H: Handlers=";

namespace {
core::map<core::string, core::vector<core::string>> get_devices_info() {
    core::map<core::string, core::vector<core::string>> result;
    auto data = core::read_file(DEVICES_INFO_PATH);

    size_t pos = 0;

    while ((pos = data.find(NAME_PREFIX, pos)) != core::string::npos) {
        pos += NAME_PREFIX.size();
        size_t name_end = data.find('"', pos);
        if (name_end == core::string::npos)
            break;

        auto name = core::string_view(data).substr(pos, name_end - pos);

        auto handlers_start = data.find(HANDLERS_PREFIX, name_end);
        if (handlers_start == core::string::npos)
            break;
        handlers_start += HANDLERS_PREFIX.size();

        auto handlers_end = data.find('\n', handlers_start);
        if (handlers_end == core::string::npos)
            break;

        auto handlers_str = core::string_view(data).substr(handlers_start, handlers_end - handlers_start);
        auto [handlers_pos, _] = result.emplace(core::string(name), core::vector<core::string>());

        for (auto handler : handlers_str / core::views::split(' '))
            handlers_pos->second.emplace_back(handler / core::acts::to<core::string>());

        pos = handlers_end;
    }

    return result;
}

core::vector<core::string> get_device_handlers(const core::string& device_name) {
    auto data = core::read_file(DEVICES_INFO_PATH);
    auto name_line = core::string(NAME_PREFIX) + device_name + '"';
    auto name_pos = data.find(name_line);

    if (name_pos == core::string::npos)
        return {};

    auto handlers_start = data.find(HANDLERS_PREFIX, name_pos);
    if (handlers_start == core::string::npos)
        return {};

    auto handlers_end = data.find('\n', handlers_start);
    if (handlers_end == core::string::npos)
        return {};

    auto handlers_str = core::string_view(data).substr(handlers_start, handlers_end - handlers_start);
    return handlers_str / core::split(' ');
}

template <typename... StrTs>
core::set<core::string> list_handlers_tmpl(StrTs&&... postfixes) {
    core::set<core::string> handlers;

    for (auto& file : fs::directory_iterator("/dev/input/by-path/")) {
        if (file.is_symlink()) {
            auto& path = file.path();
            auto strpath = path.string();
            if ((strpath.ends_with(postfixes) || ...) &&
                fs::is_character_file(path))
                handlers.insert(fs::read_symlink(path).filename().string());
        }
    }

    return handlers;
}

template <typename... StrTs>
core::vector<core::string> list_devices_tmpl(StrTs&&... postfixes) {
    auto handlers = list_handlers_tmpl(std::forward<StrTs>(postfixes)...);
    auto device_info = get_devices_info();

    core::vector<core::string> result;

    for (auto& [device_name, dev_handlers] : device_info) {
        for (auto& dev_handler : dev_handlers) {
            if (handlers.contains(dev_handler)) {
                result.push_back(device_name);
                break;
            }
        }
    }

    return result;
}

/* Empty handler_path means that device was not found */
core::string device_name_to_handler_path(const core::string& device_name) {
    auto handlers = get_device_handlers(device_name);

    if (handlers.empty())
        return {};

    auto handler_pos = handlers / core::find_if([](auto& h) { return h.starts_with("event"); });
    if (handler_pos == handlers.end())
        return {};

    return "/dev/input/" + *handler_pos;
}
}

namespace inp
{

constexpr int POLL_DELAY = 100;

void inp_linux::update_watched_devices(inp_linux* it) {
    if (it->orphan_device_receivers.empty())
        return;

    core::vector<decltype(it->orphan_device_receivers)::iterator> to_delete;

    for (auto i = it->orphan_device_receivers.begin(); i != it->orphan_device_receivers.end(); ++i) {
        auto& [device_name, receivers] = *i;
        int fd = open_device(device_name);

        if (fd != -1) {
            LOG_WARNING("Detected device {}, try to open that", device_name);
            to_delete.push_back(i);
            auto [pos, was_inserted] = it->devices.emplace(device_name, fd);
            if (was_inserted)
                it->pollfds.push_back(pollfd{fd, /* POLLIN */ 1, 0});
            it->receivers[fd] = core::move(receivers);
        }
    }

    for (auto& d : to_delete)
        it->orphan_device_receivers.erase(d);
}

void inp_linux::input_service_f(inp_linux* it) {
    using namespace core;

    bool work = it->work.load();

    vector<int> disconected_fds;
    core::optional<i32> touchpad_x;
    core::optional<i32> touchpad_y;

    core::timer timer;

    while (work) {
        if (timer.measure_count() > 2.0 /* seconds */) {
            timer.reset();
            update_watched_devices(it);
        }

        bool some_receiver_expired = false;

        //auto mtx_lock = std::lock_guard(it->mtx); // TODO: ???? overlock

        work = it->work.load();

        int rc = ::poll(reinterpret_cast<::pollfd*>(it->pollfds.data()), // NOLINT
                        static_cast<nfds_t>(it->pollfds.size()),
                        POLL_DELAY);

        if (rc <= 0)
            continue;

        for (auto& pfd : it->pollfds) {
            if (pfd.revents) {
                input_event event; // NOLINT
                ssize_t readed = ::read(pfd.fd, &event, sizeof(event));
                if (readed < static_cast<ssize_t>(sizeof(event))) {
                    auto dev = it->devices / find_if([fd = pfd.fd](auto& dev_pair) {
                                   return dev_pair.second == fd;
                               });
                    if (readed < 0) {
                        LOG_WARNING("Device {} has been disconected", dev->first);
                        disconected_fds.push_back(pfd.fd);
                    } else {
                        LOG_WARNING("Device {} produce invalid input", dev->first);
                    }
                    continue;
                }

                for (auto& receiver_weak : it->receivers.at(pfd.fd)) {
                    auto receiver = receiver_weak.lock();
                    if (!receiver) {
                        some_receiver_expired = true;
                        continue;
                    }

                    if (event.type == 1) {
                        if (event.value == 1)
                            receiver->set_press(event.code);
                        else if (event.value == 0)
                            receiver->set_release(event.code);
                    } else if (event.type == 2) {
                        if (event.code < receiver->tracks.size())
                            receiver->update_track(event.code, static_cast<trackpos>(event.value));
                    } else if (event.type == 3) {
                        /* Touchpad */
                        // TODO: rewrite this
                        if (event.code == 57 && event.value > 0)
                            touchpad_x = touchpad_y = nullopt;

                        if (event.code == 53) {
                            if (!touchpad_x) {
                                touchpad_x = event.value;
                            }
                            else {
                                receiver->update_track(
                                    0, static_cast<trackpos>(event.value - *touchpad_x));
                                touchpad_x = event.value;
                            }
                        }
                        else if (event.code == 54) {
                            if (!touchpad_y) {
                                touchpad_y = event.value;
                            }
                            else {
                                receiver->update_track(
                                    1, static_cast<trackpos>(event.value - *touchpad_y));
                                touchpad_y = event.value;
                            }
                        }
                    }
                }
            }
        }

        if (!disconected_fds.empty()) {
            it->orphanize_fds(disconected_fds);
            disconected_fds.clear();
        }

        if (some_receiver_expired)
            it->remove_unused();

        //std::this_thread::yield();
    }
}

core::string inp_linux::default_keyboard_device() {
    auto keyboards = list_keyboard_devices();
    if (keyboards.empty())
        throw std::runtime_error("There are no keyboard devices!");
    return keyboards.front();
}

core::string inp_linux::default_mouse_device() {
    auto mouses = list_mouse_devices();

    if (mouses.empty())
        throw std::runtime_error("There are no mouse devices!");

    auto not_touchpad = mouses / core::find_if([](auto& d) {
                                     return d.find("Touchpad") == core::string::npos &&
                                            d.find("touchpad") == core::string::npos;
                                 });
    if (not_touchpad != mouses.end())
        return *not_touchpad;
    else
        return mouses.front();
}

core::vector<core::string> inp_linux::list_devices() {
    using namespace core;
    return list_devices_tmpl("event-kbd"sv, "event-mouse"sv);
}

core::vector<core::string> inp_linux::list_keyboard_devices() {
    using namespace core;
    return list_devices_tmpl("event-kbd"sv);
}

core::vector<core::string> inp_linux::list_mouse_devices() {
    using namespace core;
    return list_devices_tmpl("event-mouse"sv);
}

int inp_linux::open_device(const core::string& device_name) {
    auto handler_path = device_name_to_handler_path(device_name);
    int fd = -1;
    if (!handler_path.empty())
        fd = ::open(handler_path.data(), O_RDONLY | O_NONBLOCK); // NOLINT
    return fd;
}

void inp_linux::close_device(int fd) {
    ::close(fd);
}

void inp_linux::start() {
    work = true;
    running_thread = std::thread([this]{
        input_service_f(this);
    });
}

void inp_linux::stop() {
    work = false;
    running_thread.join();
}

} // namespace inp

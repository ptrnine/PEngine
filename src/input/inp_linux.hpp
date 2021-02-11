#pragma once
#include "core/types.hpp"
#include "core/helper_macros.hpp"
#include "core/async.hpp"
#include "inp_event_receiver.hpp"

namespace inp {
    class inp_linux {
        SINGLETON_IMPL(inp_linux);

        struct pollfd {
            int fd;
            short events;
            short revents;
            friend auto operator<=>(const pollfd&, const pollfd&) = default;
        };

    public:
        static core::string default_keyboard_device();
        static core::string default_mouse_device();
        static core::vector<core::string> list_mouse_devices();
        static core::vector<core::string> list_keyboard_devices();
        static core::vector<core::string> list_devices();

        static int open_device(const core::string& device_name);
        static void close_device(int fd);

        core::shared_ptr<inp_event_receiver> create_keyboard_mouse_receiver() {
            return create_receiver(default_keyboard_device(), default_mouse_device());
        }

        template <core::StringLike... StrTs>
        core::shared_ptr<inp_event_receiver> create_receiver(StrTs&&... devices_names) {
            static_assert(sizeof...(StrTs) > 0);

            auto lock_guard = std::lock_guard(mtx);

            if (work)
                stop();

            auto event_receiver = inp_event_receiver::create_shared();

            auto ts = [this, &event_receiver](auto&& device_name) {
                int fd = open_device(device_name);

                if (fd != -1) {
                    auto [pos, was_inserted] = devices.emplace(device_name, fd);
                    if (was_inserted)
                        pollfds.push_back(pollfd{fd, /* POLLIN */ 1, 0});
                    receivers[fd].push_back(event_receiver);
                }
                else
                    orphan_device_receivers[device_name].push_back(event_receiver);
            };

            (ts(devices_names), ...);

            start();

            return event_receiver;

        }

    private:
        static void update_watched_devices(inp_linux* it);
        static void input_service_f(inp_linux* instance);

        void start();
        void stop();

        void remove_fds(const core::vector<int>& fds) {
            for (auto fd : fds) {
                receivers.erase(fd);
                devices.erase(std::find_if(devices.begin(), devices.end(), [fd](const auto& kv) {
                    return kv.second == fd;
                }));

                pollfds.erase(std::remove_if(
                    pollfds.begin(), pollfds.end(), [fd](auto& pfd) { return pfd.fd == fd; }));

                close_device(fd);
            }
        }

        void orphanize_fds(const core::vector<int>& fds) {
            for (auto fd : fds) {
                auto recvs = core::move(receivers.at(fd));
                receivers.erase(fd);

                auto found_dev = std::find_if(devices.begin(), devices.end(), [fd](const auto& kv) {
                    return kv.second == fd;
                });
                auto device_name = found_dev->first;
                devices.erase(found_dev);

                pollfds.erase(std::remove_if(
                    pollfds.begin(), pollfds.end(), [fd](auto& pfd) { return pfd.fd == fd; }));

                close_device(fd);

                orphan_device_receivers.emplace(core::move(device_name), core::move(recvs));
            }
        }

        void remove_unused() {
            core::vector<decltype(receivers)::value_type::second_type::iterator> delete_recs;
            core::vector<int> delete_fds;
            for (auto& [fd, recs] : receivers) {
                delete_recs.clear();

                for (auto i = recs.begin(); i != recs.end(); ++i)
                    if (i->expired())
                        delete_recs.push_back(i);

                if (delete_recs.size() == recs.size())
                    delete_fds.push_back(fd);
                else
                    for (auto& rec_iterator : delete_recs)
                        recs.erase(rec_iterator);
            }
            remove_fds(delete_fds);
        }

    private:
        inp_linux() = default;

        ~inp_linux() {
            stop();
            for (auto& pollfd : pollfds)
                close_device(pollfd.fd);
        }

    private:
        core::map<core::string, int>                                     devices;
        core::map<int, core::vector<core::weak_ptr<inp_event_receiver>>> receivers;
        core::map<core::string, core::vector<core::weak_ptr<inp_event_receiver>>>
                   orphan_device_receivers;
        std::mutex mtx;

        std::thread          running_thread;
        core::vector<pollfd> pollfds;
        std::atomic<bool>    work = false;
    };

    inline inp_linux& input() {
        return inp_linux::instance();
    }
}

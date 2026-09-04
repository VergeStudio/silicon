module;

#if defined(SILICON_PLATFORM_WINDOWS)
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <coroutine>
#include <map>
#include <optional>

module silicon.scheduler;


#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :poll_info;
import :poll_info_impl;

#if defined(SILICON_PLATFORM_WINDOWS)
using namespace std::chrono_literals;

namespace silicon::scheduler {



struct timer_post_ctx {
    HANDLE iocp;
    poll_info *pi;
};



static void CALLBACK timer_post_callback(PTP_CALLBACK_INSTANCE, void *ctx, PTP_TIMER) noexcept {
    auto *tc = static_cast<timer_post_ctx *>(ctx);
    PostQueuedCompletionStatus(tc->iocp, 0, reinterpret_cast<ULONG_PTR>(tc->pi), nullptr);
}






struct io_notifier::impl {

    HANDLE m_iocp{};


    bool m_valid{false};


    std::mutex m_mutex;


    struct watch_entry {
        poll_op op;
        void *data;
        bool keep;
        bool is_cancel_event;
    };
    std::unordered_map<fd_t, watch_entry> m_watched_fds;





    timer_post_ctx m_timer_ctx{};
    PTP_TIMER m_tp_timer{nullptr};
};





io_notifier::io_notifier()
    : m_p(std::make_unique<impl>()) {
    m_p->m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);

    m_p->m_valid = (m_p->m_iocp != nullptr);
}

bool io_notifier::is_valid() const noexcept {
    return m_p->m_valid;
}

io_notifier::~io_notifier() {
    if (m_p->m_tp_timer != nullptr) {

        SetThreadpoolTimer(m_p->m_tp_timer, nullptr, 0, 0);
        WaitForThreadpoolTimerCallbacks(m_p->m_tp_timer, TRUE);
        CloseThreadpoolTimer(m_p->m_tp_timer);
        m_p->m_tp_timer = nullptr;
    }
}

void io_notifier::remove_fd(fd_t fd) {
    std::lock_guard lock(m_p->m_mutex);
    m_p->m_watched_fds.erase(fd);
}

bool io_notifier::watch(fd_t fd, poll_op op, void *data, bool keep, bool is_cancel_event) {
    std::lock_guard lock(m_p->m_mutex);
    m_p->m_watched_fds[fd] = {op, data, keep, is_cancel_event};
    return true;
}

bool io_notifier::watch(poll_info &pi) {
    watch(pi.m_p->m_fd, pi.m_p->m_op, static_cast<void *>(&pi), false, false);

    if (pi.m_p->m_cancel_trigger.has_value()) {
        watch(pi.m_p->m_cancel_trigger.value().native_handle(), poll_op::read,
              static_cast<void *>(&pi), false, true);
    }

    return true;
}

bool io_notifier::unwatch(fd_t fd, poll_op) {
    remove_fd(fd);
    return true;
}

bool io_notifier::unwatch(poll_info &pi) {
    remove_fd(pi.m_p->m_fd);
    if (pi.m_p->m_cancel_trigger.has_value()) {
        remove_fd(pi.m_p->m_cancel_trigger.value().native_handle());
    }
    return true;
}

bool io_notifier::watch_timer(const timer_handle &timer, std::chrono::nanoseconds duration) {







    auto *pi = reinterpret_cast<poll_info *>(const_cast<void *>(timer.get_inner()));
    if (!pi || !m_p->m_valid) {
        return false;
    }

    if (duration < 0ns) {
        duration = 0ns;
    }

    m_p->m_timer_ctx.iocp = m_p->m_iocp;
    m_p->m_timer_ctx.pi = pi;

    if (m_p->m_tp_timer == nullptr) {
        m_p->m_tp_timer = CreateThreadpoolTimer(timer_post_callback, &m_p->m_timer_ctx, nullptr);
        if (!m_p->m_tp_timer) {
            return false;
        }
    }


    LARGE_INTEGER liDueTime{};
    liDueTime.QuadPart = -static_cast<LONGLONG>((duration.count() + 99) / 100);
    FILETIME ft{};
    ft.dwHighDateTime = static_cast<DWORD>(static_cast<std::uint64_t>(liDueTime.QuadPart) >> 32);
    ft.dwLowDateTime = static_cast<DWORD>(static_cast<std::uint64_t>(liDueTime.QuadPart) & 0xFFFFFFFFu);

    SetThreadpoolTimer(m_p->m_tp_timer, &ft, 0, 0);
    return true;
}

bool io_notifier::unwatch_timer(const timer_handle &) {

    if (m_p->m_tp_timer != nullptr) {
        SetThreadpoolTimer(m_p->m_tp_timer, nullptr, 0, 0);
    }
    return true;
}

void io_notifier::next_events(
        std::vector<std::pair<poll_info *, poll_status>> &ready_events,
        std::chrono::milliseconds timeout
) {



    using steady_clock = std::chrono::steady_clock;
    const auto deadline = steady_clock::now() + timeout;
    auto remaining_ms = [&deadline]() -> DWORD {
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - steady_clock::now());
        return left.count() > 0 ? static_cast<DWORD>(left.count()) : 0;
    };


    auto drain_packets = [&](DWORD wait_ms) {
        DWORD bytes_transferred;
        ULONG_PTR completion_key;
        LPOVERLAPPED overlapped;
        while (GetQueuedCompletionStatus(
                m_p->m_iocp, &bytes_transferred, &completion_key, &overlapped, wait_ms)) {




            auto *pi = reinterpret_cast<poll_info *>(completion_key);
            if (pi) {
                ready_events.emplace_back(pi, poll_status::timeout);
            }
            wait_ms = 0;
        }
    };


    drain_packets(0);



    std::vector<WSAPOLLFD> poll_fds;
    std::vector<poll_info *> poll_info_map;

    {
        std::lock_guard lock(m_p->m_mutex);
        for (auto &[fd, entry] : m_p->m_watched_fds) {
            if (fd < 0) continue;
            if (entry.is_cancel_event) continue;

            short events = 0;
            if (poll_op_readable(entry.op)) events |= POLLRDNORM;
            if (poll_op_writeable(entry.op)) events |= POLLWRNORM;

            poll_fds.push_back({static_cast<SOCKET>(fd), events, 0});
            poll_info_map.push_back(static_cast<poll_info *>(entry.data));
        }
    }


    if (!poll_fds.empty()) {
        int poll_timeout = static_cast<int>(remaining_ms());

        int result = WSAPoll(poll_fds.data(), static_cast<ULONG>(poll_fds.size()), poll_timeout);
        if (result == SOCKET_ERROR && ready_events.empty()) {



            drain_packets(remaining_ms());
            return;
        }
        if (result > 0) {
            for (size_t i = 0; i < poll_fds.size(); ++i) {
                auto &pfd = poll_fds[i];
                auto *pi = poll_info_map[i];

                if (pfd.revents & POLLRDNORM) {
                    if (!pi->m_p->m_processed) {
                        pi->m_p->m_processed = true;
                        ready_events.emplace_back(pi, poll_status::read);
                    }
                } else if (pfd.revents & POLLWRNORM) {
                    if (!pi->m_p->m_processed) {
                        pi->m_p->m_processed = true;
                        ready_events.emplace_back(pi, poll_status::write);
                    }
                } else if (pfd.revents & POLLHUP) {
                    if (!pi->m_p->m_processed) {
                        pi->m_p->m_processed = true;
                        ready_events.emplace_back(pi, poll_status::closed);
                    }
                } else if (pfd.revents & POLLERR) {
                    if (!pi->m_p->m_processed) {
                        pi->m_p->m_processed = true;
                        ready_events.emplace_back(pi, poll_status::error);
                    }
                } else if (pfd.revents & POLLNVAL) {
                    if (!pi->m_p->m_processed) {
                        pi->m_p->m_processed = true;
                        ready_events.emplace_back(pi, poll_status::error);
                    }
                }
            }
        }


        drain_packets(0);
    } else if (ready_events.empty()) {

        drain_packets(remaining_ms());
    }
}

bool io_notifier::post(void *data) {
    if(!m_p->m_valid) { return false; }





    return ::PostQueuedCompletionStatus(
                   m_p->m_iocp, 0, reinterpret_cast<ULONG_PTR>(data), nullptr
           ) != 0;
}

auto io_notifier::native_handle() const -> HANDLE {
    return m_p->m_iocp;
}

}
#endif

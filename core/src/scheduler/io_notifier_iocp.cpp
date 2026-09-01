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
// MSVC 须显式 import 本模块接口方可访问其导出实体；clang 与标准不允许
// 实现单元自引用，故以 _MSC_VER 守卫（跨编译器分歧，ddbb1aa 实战）。
#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :poll_info;
import :poll_info_impl;
#include "poll_info_impl.hpp"

#if defined(SILICON_PLATFORM_WINDOWS)
using namespace std::chrono_literals;

// 复用 silicon.coroutine 的基础 I/O 类型（不 export，仅本单元内简化书写）。
namespace silicon::scheduler {
using silicon::coroutine::fd_t;
using silicon::coroutine::poll_op;
using silicon::coroutine::poll_op_readable;
using silicon::coroutine::poll_op_writeable;
using silicon::coroutine::poll_status;
using silicon::coroutine::poll_stop_token;
using silicon::coroutine::time_point;
} // namespace silicon::scheduler

namespace silicon::scheduler {

// ---------------------------------------------------------------------------
// PIMPL: platform-specific implementation state for io_notifier (IOCP backend).
// Defined here (where it is complete) so the unique_ptr destructor and every
// method that touches these members can be compiled.
// ---------------------------------------------------------------------------
struct io_notifier::impl {
    /// The IOCP handle.
    HANDLE m_iocp{};

    /// Mutex protecting the watched-fds tracking structures.
    std::mutex m_mutex;

    /// Currently watched socket file descriptors and their watch mode.
    struct watch_entry {
        poll_op op;
        void *data;
        bool keep;
        bool is_cancel_event;
    };
    std::unordered_map<fd_t, watch_entry> m_watched_fds;
};

// ---------------------------------------------------------------------------
// Timer callback — posted to the IOCP when a timer fires
// ---------------------------------------------------------------------------

struct timer_completion_key {
    poll_info *pi;
    bool fired;
};

static void CALLBACK timer_callback(PTP_CALLBACK_INSTANCE, void *ctx, PTP_TIMER timer) noexcept {
    auto *tck = static_cast<timer_completion_key *>(ctx);
    tck->fired = true;
    // Post the timer event to the IOCP. The io_notifier pointer is embedded in the
    // first pointer, poll_info in the second. fd_t 为 int：整型→指针须经 uintptr_t
    // 中转 reinterpret_cast。
    HANDLE iocp = reinterpret_cast<HANDLE>(static_cast<std::uintptr_t>(tck->pi->m_p->m_fd));
    PostQueuedCompletionStatus(iocp, 0, reinterpret_cast<ULONG_PTR>(tck->pi), nullptr);
    CloseThreadpoolTimer(timer);
}

// ---------------------------------------------------------------------------
// io_notifier (IOCP backend)
// ---------------------------------------------------------------------------

io_notifier::io_notifier()
    : m_p(std::make_unique<impl>()) {
    m_p->m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!m_p->m_iocp) {
        throw std::system_error(GetLastError(), std::system_category(),
                                "io_notifier: CreateIoCompletionPort failed");
    }
}

io_notifier::~io_notifier() = default;

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
    // Store the IOCP handle in m_fd so the timer callback can reach it.
    // NOTE: this is a simplified approach — in production the io_notifier would
    // track timer keys directly.
    auto *pi = reinterpret_cast<poll_info *>(const_cast<void *>(timer.get_inner()));
    if (!pi) return false;

    HANDLE hTimer = CreateWaitableTimerW(nullptr, TRUE, nullptr);
    if (!hTimer) {
        return false;
    }

    LARGE_INTEGER liDueTime{};
    auto ns = duration.count();
    liDueTime.QuadPart = -static_cast<LONGLONG>(ns / 100); // convert to 100-ns intervals, negative = relative

    if (!SetWaitableTimer(hTimer, &liDueTime, 0, nullptr, nullptr, FALSE)) {
        CloseHandle(hTimer);
        return false;
    }

    // Associate the waitable timer with the IOCP
    if (!CreateIoCompletionPort(hTimer, m_p->m_iocp, reinterpret_cast<ULONG_PTR>(pi), 0)) {
        CloseHandle(hTimer);
        return false;
    }

    return true;
}

bool io_notifier::unwatch_timer(const timer_handle &) {
    // In this implementation, timers fire once and self-clean.
    return true;
}

void io_notifier::next_events(
        std::vector<std::pair<poll_info *, poll_status>> &ready_events,
        std::chrono::milliseconds timeout
) {
    // Phase 1: Drain any IOCP completions (timer expirations, etc.)
    DWORD bytes_transferred;
    ULONG_PTR completion_key;
    LPOVERLAPPED overlapped;
    DWORD timeout_ms = static_cast<DWORD>(timeout.count());

    auto wait_start = std::chrono::steady_clock::now();

    // Check for IOCP completions (from timers)
    while (true) {
        BOOL ok = GetQueuedCompletionStatus(
                m_p->m_iocp, &bytes_transferred, &completion_key, &overlapped, 0);
        if (!ok) {
            DWORD err = GetLastError();
            if (err == WAIT_TIMEOUT) {
                break; // No more completions
            }
            // Error or the completion port handle is closed
            break;
        }

        // completion_key is a poll_info* posted by the timer callback
        auto *pi = reinterpret_cast<poll_info *>(completion_key);
        if (pi) {
            if (!pi->m_p->m_processed) {
                pi->m_p->m_processed = true;
                pi->m_p->m_poll_status = poll_status::timeout;
                ready_events.emplace_back(pi, poll_status::timeout);
            }
        }
    }

    // Phase 2: Build WSAPoll fd set from watched fds
    // Only include "real" socket fds (exclude cancel-event pipe fds from poll_stop_source)
    std::vector<WSAPOLLFD> poll_fds;
    std::vector<poll_info *> poll_info_map; // parallel to poll_fds

    {
        std::lock_guard lock(m_p->m_mutex);
        for (auto &[fd, entry] : m_p->m_watched_fds) {
            if (fd < 0) continue;
            if (entry.is_cancel_event) continue; // skip cancel pipe fds; they can't be WSAPoll'd

            short events = 0;
            if (poll_op_readable(entry.op)) events |= POLLRDNORM;
            if (poll_op_writeable(entry.op)) events |= POLLWRNORM;

            poll_fds.push_back({static_cast<SOCKET>(fd), events, 0});
            poll_info_map.push_back(static_cast<poll_info *>(entry.data));
        }
    }

    // Phase 3: WSAPoll for socket readiness
    if (!poll_fds.empty()) {
        // Calculate remaining timeout (don't exceed the original timeout)
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - wait_start);
        int poll_timeout = static_cast<int>((timeout > elapsed) ? (timeout - elapsed).count() : 0);

        int result = WSAPoll(poll_fds.data(), static_cast<ULONG>(poll_fds.size()), poll_timeout);
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
                } else if (pfd.revents & POLLHUP) { // Windows WSAPoll 无 POLLRDHUP
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
    }
}

auto io_notifier::native_handle() const -> HANDLE {
    return m_p->m_iocp;
}

} // namespace silicon::scheduler
#endif

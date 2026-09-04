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
// 实现单元自引用，故以 _MSC_VER 守卫。
#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :poll_info;
import :poll_info_impl;

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

// threadpool 定时器到期回调的上下文（文件局部；impl 为 private 嵌套类，
// 自由回调函数不可引用其嵌套类型）。
struct timer_post_ctx {
    HANDLE iocp;
    poll_info *pi;
};

// threadpool 定时器到期回调：向 IOCP 投递完成包（key = 哨兵 poll_info*），
// 事件循环线程经 GetQueuedCompletionStatus 取到后走 process_timeout_execute。
static void CALLBACK timer_post_callback(PTP_CALLBACK_INSTANCE, void *ctx, PTP_TIMER) noexcept {
    auto *tc = static_cast<timer_post_ctx *>(ctx);
    PostQueuedCompletionStatus(tc->iocp, 0, reinterpret_cast<ULONG_PTR>(tc->pi), nullptr);
}

// ---------------------------------------------------------------------------
// PIMPL: platform-specific implementation state for io_notifier (IOCP backend).
// Defined here (where it is complete) so the unique_ptr destructor and every
// method that touches these members can be compiled.
// ---------------------------------------------------------------------------
struct io_notifier::impl {
    /// The IOCP handle.
    HANDLE m_iocp{};

    /// Validity flag: true only when the IOCP handle was created successfully.
    bool m_valid{false};

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

    /// 常驻 threadpool 定时器：到期回调经 PostQueuedCompletionStatus 把完成包
    /// （key = 哨兵 poll_info*）投递到 IOCP。IOCP 只接受支持 overlapped I/O
    /// 的对象句柄，waitable timer 不可关联（CreateIoCompletionPort 报
    /// ERROR_INVALID_HANDLE），故须借 threadpool 定时器转发。
    timer_post_ctx m_timer_ctx{};
    PTP_TIMER m_tp_timer{nullptr};
};

// ---------------------------------------------------------------------------
// io_notifier (IOCP backend)
// ---------------------------------------------------------------------------

io_notifier::io_notifier()
    : m_p(std::make_unique<impl>()) {
    m_p->m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    // 构造不再抛异常：失败时 m_valid 保持 false，由调用方通过 is_valid() 检查。
    m_p->m_valid = (m_p->m_iocp != nullptr);
}

bool io_notifier::is_valid() const noexcept {
    return m_p->m_valid;
}

io_notifier::~io_notifier() {
    if (m_p->m_tp_timer != nullptr) {
        // 停止并等待未决回调，防止析构后回调访问已销毁的 ctx/IOCP。
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
    // 与 kqueue/epoll 的 EV_ONESHOT / timerfd 语义对齐：每次 watch_timer 重新
    // 武装一次性到期（period=0），到期回调向 IOCP 投递完成包（key = 哨兵
    // poll_info*，即 io_scheduler 的 &m_timer_poll）。
    //
    // 不能用 CreateWaitableTimer + CreateIoCompletionPort：IOCP 只接受支持
    // overlapped I/O 的对象句柄，waitable timer 不可关联（报
    // ERROR_INVALID_HANDLE(6)），故借 threadpool 定时器转发完成包。
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

    // 相对到期时间：FILETIME 负值 = 相对时刻，单位 100ns，向上取整。
    LARGE_INTEGER liDueTime{};
    liDueTime.QuadPart = -static_cast<LONGLONG>((duration.count() + 99) / 100);
    FILETIME ft{};
    ft.dwHighDateTime = static_cast<DWORD>(static_cast<std::uint64_t>(liDueTime.QuadPart) >> 32);
    ft.dwLowDateTime = static_cast<DWORD>(static_cast<std::uint64_t>(liDueTime.QuadPart) & 0xFFFFFFFFu);

    SetThreadpoolTimer(m_p->m_tp_timer, &ft, 0, 0);
    return true;
}

bool io_notifier::unwatch_timer(const timer_handle &) {
    // 常驻 threadpool 定时器仅取消未决到期，不销毁（下次 watch_timer 复用）。
    if (m_p->m_tp_timer != nullptr) {
        SetThreadpoolTimer(m_p->m_tp_timer, nullptr, 0, 0);
    }
    return true;
}

void io_notifier::next_events(
        std::vector<std::pair<poll_info *, poll_status>> &ready_events,
        std::chrono::milliseconds timeout
) {
    // 与 epoll_wait / kevent 的阻塞语义对齐：本调用应在 timeout 内等待事件就绪，
    // 而非立即返回。manual 模式下调用方（io_scheduler::process_events）仅靠本
    // 函数消耗时间，若立即返回，定时器事件将永远等不到驱动。
    using steady_clock = std::chrono::steady_clock;
    const auto deadline = steady_clock::now() + timeout;
    auto remaining_ms = [&deadline]() -> DWORD {
        auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - steady_clock::now());
        return left.count() > 0 ? static_cast<DWORD>(left.count()) : 0;
    };

    // 排空完成包：wait_ms > 0 时阻塞等待首个包，其后一律 0 超时排空积压。
    auto drain_packets = [&](DWORD wait_ms) {
        DWORD bytes_transferred;
        ULONG_PTR completion_key;
        LPOVERLAPPED overlapped;
        while (GetQueuedCompletionStatus(
                m_p->m_iocp, &bytes_transferred, &completion_key, &overlapped, wait_ms)) {
            // completion_key 是 watch_timer 关联定时器时的 poll_info*（哨兵
            // &m_timer_poll）。不得在此写 m_processed：哨兵 poll_info 常驻复用，
            // 一次性置位会吞掉后续所有定时器事件；去重由 io_scheduler 的
            // process_event_execute / process_timeout_execute 负责（与 kqueue 一致）。
            auto *pi = reinterpret_cast<poll_info *>(completion_key);
            if (pi) {
                ready_events.emplace_back(pi, poll_status::timeout);
            }
            wait_ms = 0;
        }
    };

    // Phase 1: 非阻塞排空已积压的完成包（timer 到期等）。
    drain_packets(0);

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
        int poll_timeout = static_cast<int>(remaining_ms());

        int result = WSAPoll(poll_fds.data(), static_cast<ULONG>(poll_fds.size()), poll_timeout);
        if (result == SOCKET_ERROR && ready_events.empty()) {
            // fd 集中含非 socket fd（如调度管道的 CRT fd）时 WSAPoll 立即以
            // WSAENOTSOCK 失败——此时没有任何 socket 可等，回退为阻塞等待
            // IOCP 完成包（timer 哨兵）至 deadline，维持等待语义。
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

        // socket 等待结束后再排空一次等待期间到达的完成包。
        drain_packets(0);
    } else if (ready_events.empty()) {
        // 无 socket 可等：阻塞等待 IOCP 完成包（timer 哨兵）至 deadline。
        drain_packets(remaining_ms());
    }
}

bool io_notifier::post(void *data) {
    if(!m_p->m_valid) { return false; }
    // 投递一条「假完成」：completion_key = 调用方给的哨兵（io_scheduler 的
    // m_completion_ptr）。事件循环线程经 GetQueuedCompletionStatus 取到后，
    // process_events_execute 按 handle_ptr==m_completion_ptr 分派到
    // drain_ring_completions。这是 Windows 唯一可靠的跨线程唤醒通道（设计 R4）：
    // 不能写 CRT schedule pipe，因为 IOCP 等待语义无法被普通管道写打断。
    return ::PostQueuedCompletionStatus(
                   m_p->m_iocp, 0, reinterpret_cast<ULONG_PTR>(data), nullptr
           ) != 0;
}

auto io_notifier::native_handle() const -> HANDLE {
    return m_p->m_iocp;
}

} // namespace silicon::scheduler
#endif

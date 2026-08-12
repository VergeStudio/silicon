module;

#if defined(_WIN32)
#    include <Windows.h> // GetLastError
#else
#    include <sys/socket.h>
#    include <sys/types.h>
#    include <unistd.h>
#endif
// 迁出 silicon.coroutine 后不再借道该模块 GMF 间接获得这些标准头，
// 本实现单元用到的标准设施一律在此显式引入。
#include <array>
#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstring>
#include <exception> // std::exception：io_scheduler::create() 捕获构造期异常
#include <functional> // std::function 与 nullptr 比较所需的 operator==
#include <system_error> // std::system_category：替代被 MSVC 弃用的 strerror
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
#include <expected>


module silicon.scheduler;

import :poll_info_impl;





using namespace std::chrono_literals;
// 与 io_scheduler.cppm 一致：沿用 coroutine 基础类型的非限定名。
using namespace silicon::coroutine;

namespace silicon::scheduler {


static auto
make_spawned_joinable_wait_task(std::unique_ptr<silicon::scheduler::task_group<silicon::scheduler::io_scheduler>> group_ptr) -> silicon::scheduler::task<void> {
    co_await *group_ptr;
    co_return;
}



io_scheduler::io_scheduler(options &&opts, private_constructor)
    : m_p(std::make_unique<Impl>(std::move(opts))) {
    if(!m_p->m_io_notifier.watch(m_p->m_shutdown_pipe.read_fd(), silicon::coroutine::poll_op::read, const_cast<void *>(m_shutdown_ptr), true)) {
        throw std::runtime_error("Failed to register m_shutdown_pipe.read_fd() for read events.");
    }

    if(!m_p->m_io_notifier.watch(m_p->m_schedule_pipe.read_fd(), silicon::coroutine::poll_op::read, const_cast<void *>(m_schedule_ptr), true)) {
        throw std::runtime_error("Failed to register m_schedule.pipe.read_rd() for read events.");
    }

    m_p->m_recent_events.reserve(m_max_events);

    if(m_p->m_opts.execution_strategy == execution_strategy_t::process_tasks_on_thread_pool) {
        m_p->m_thread_pool = thread_pool::create(std::move(m_p->m_opts.pool)).value();
    }
}

auto io_scheduler::create(options opts) -> std::expected<std::unique_ptr<io_scheduler>, std::error_code> {
    try {
        auto s = std::make_unique<io_scheduler>(std::move(opts), private_constructor{});

        // Spawn the dedicated event loop thread once the scheduler is fully constructed
        // so it has a full object to work with.
        if(s->m_p->m_opts.thread_strategy == thread_strategy_t::spawn) {
            s->m_p->m_io_thread = std::thread([s = s.get()]() { s->process_events_dedicated_thread(); });
        }
        // else manual mode, the user must call process_events.

        return s;
    } catch(const std::exception &) {
        // 构造期失败（事件管道创建 / fd 注册 / 线程池初始化）统一收敛为 unexpected。
        // 具体失败原因由底层 ctor 的 stderr 诊断信息保留；此处仅给出模块级错误码。
        return std::unexpected(make_error_code(scheduler_error::kUnknown));
    }
}

io_scheduler::~io_scheduler() {
    shutdown();

    if(m_p->m_io_thread.joinable()) {
        m_p->m_io_thread.join();
    }

    m_p->m_shutdown_pipe.close();
    m_p->m_schedule_pipe.close();
}

auto io_scheduler::process_events(std::chrono::milliseconds timeout) -> std::size_t {
    process_events_manual(timeout);
    return size();
}

auto io_scheduler::spawn_detached(silicon::scheduler::task<void> &&task) -> bool {
    m_p->m_size.fetch_add(1, std::memory_order::release);
    auto wrapper_task = silicon::scheduler::make_task_self_deleting(std::move(task));
    wrapper_task.promise().user_final_suspend([this]() -> void { m_p->m_size.fetch_sub(1, std::memory_order::release); });
    return resume(wrapper_task.handle());
}

auto io_scheduler::spawn_joinable(silicon::scheduler::task<void> &&task) -> silicon::scheduler::task<void> {
    auto group_ptr = std::make_unique<silicon::scheduler::task_group<silicon::scheduler::io_scheduler>>(this, std::move(task));
    return make_spawned_joinable_wait_task(std::move(group_ptr));
}

auto io_scheduler::schedule_at(time_point time) -> silicon::scheduler::task<void> {
    return yield_until(time);
}

auto io_scheduler::yield_until(time_point time) -> silicon::scheduler::task<void> {
    auto now = clock::now();

    // If the requested time is in the past (or now!) bail out!
    if(time <= now) {
        co_await schedule();
    } else {
        m_p->m_size.fetch_add(1, std::memory_order::release);

        auto amount = std::chrono::duration_cast<std::chrono::milliseconds>(time - now);

        silicon::scheduler::poll_info pi{};
        add_timer_token(now + amount, pi);
        co_await pi;
    }
    co_return;
}

auto io_scheduler::poll(
        fd_t fd,
        silicon::coroutine::poll_op op,
        std::chrono::milliseconds timeout,
        std::optional<poll_stop_token> cancel_trigger
) -> silicon::scheduler::task<poll_status> {
    // Because the size will drop when this coroutine suspends every poll needs to undo the subtraction
    // on the number of active tasks in the scheduler.  When this task is resumed by the event loop.
    m_p->m_size.fetch_add(1, std::memory_order::release);

    // Setup two events, a timeout event and the actual poll for op event.
    // Whichever triggers first will delete the other to guarantee only one wins.
    // The resume token will be set by the scheduler to what the event turned out to be.

    bool timeout_requested = (timeout > 0ms);

    auto pi = silicon::scheduler::poll_info{fd, op, cancel_trigger};

    if(timeout_requested) {
        pi.m_p->m_timer_pos = add_timer_token(clock::now() + timeout, pi);
    }

    if(!m_p->m_io_notifier.watch(pi)) {
        std::cerr << "Failed to add " << fd << " to watch list\n";
    }

    // The event loop will 'clean-up' whichever event didn't win since the coroutine is scheduled
    // onto the thread poll its possible the other type of event could trigger while its waiting
    // to execute again, thus restarting the coroutine twice, that would be quite bad.
    auto result = co_await pi;
    co_return result;
}

auto io_scheduler::resume(std::coroutine_handle<> handle) -> bool {
    if(handle == nullptr || handle.done()) {
        return false;
    }

    if(m_p->m_shutdown_requested.load(std::memory_order::acquire)) {
        return false;
    }

    if(m_p->m_opts.execution_strategy == execution_strategy_t::process_tasks_inline) {
        auto *schedule_op = new schedule_operation{*this};
        schedule_op->m_allocated = true;
        schedule_op->await_suspend(handle);
        return true;
    } else {
        return m_p->m_thread_pool->resume(handle);
    }
}

auto io_scheduler::shutdown() noexcept -> void {
    // Only allow shutdown to occur once.
    if(m_p->m_shutdown_requested.exchange(true, std::memory_order::acq_rel) == false) {
        // Signal the event loop to stop asap.
        const constexpr int value{1};
        long written = m_p->m_shutdown_pipe.write(&value, sizeof(value));
        if(written != sizeof(value)) {
            std::cerr << "silicon::scheduler::io_scheduler::shutdown() failed to write to shutdown pipe, bytes written=" << written
                      << "\n";
        }

        if(m_p->m_io_thread.joinable()) {
            m_p->m_io_thread.join();
        }

        if(m_p->m_thread_pool != nullptr) {
            m_p->m_thread_pool->shutdown();
        }
    }
}

auto io_scheduler::yield_for_internal(std::chrono::nanoseconds amount) -> silicon::scheduler::task<void> {
    if(amount <= 0ms) {
        co_await schedule();
    } else {
        // Yield/timeout tasks are considered live in the scheduler and must be accounted for. Note
        // that if the user gives an invalid amount and schedule() is directly called it will account
        // for the scheduled task there.
        m_p->m_size.fetch_add(1, std::memory_order::release);

        // Yielding does not require setting the timer position on the poll info since
        // it doesn't have a corresponding 'event' that can trigger, it always waits for
        // the timeout to occur before resuming.

        silicon::scheduler::poll_info pi{};
        add_timer_token(clock::now() + amount, pi);
        co_await pi;
    }
    co_return;
}

auto io_scheduler::process_events_manual(std::chrono::milliseconds timeout) -> void {
    bool expected{false};
    if(m_p->m_io_processing.compare_exchange_strong(expected, true, std::memory_order::release, std::memory_order::relaxed)) {
        process_events_execute(timeout);
        m_p->m_io_processing.exchange(false, std::memory_order::release);
    }
}

auto io_scheduler::process_events_dedicated_thread() -> void {
    if(m_p->m_opts.on_io_thread_start_functor != nullptr) {
        m_p->m_opts.on_io_thread_start_functor();
    }

    m_p->m_io_processing.exchange(true, std::memory_order::release);
    // Execute tasks until stopped or there are no more tasks to complete.
    while(!m_p->m_shutdown_requested.load(std::memory_order::acquire) || size() > 0) {
        process_events_execute(m_default_timeout);
    }
    m_p->m_io_processing.exchange(false, std::memory_order::release);

    if(m_p->m_opts.on_io_thread_stop_functor != nullptr) {
        m_p->m_opts.on_io_thread_stop_functor();
    }
}

auto io_scheduler::process_events_execute(std::chrono::milliseconds timeout) -> void {
    // Clear the recent events without decreasing the allocated capacity to reduce allocations
    m_p->m_recent_events.clear();
    m_p->m_io_notifier.next_events(m_p->m_recent_events, timeout);

    for(auto &[handle_ptr, poll_status]: m_p->m_recent_events) {
        if(handle_ptr == m_timer_ptr) {
            // Process all events that have timed out.
            process_timeout_execute();
        } else if(handle_ptr == m_schedule_ptr) {
            // Process scheduled coroutines.
            process_scheduled_execute_inline();
        } else if(handle_ptr == m_shutdown_ptr) [[unlikely]] {
            // Nothing to do, just needed to wake-up and smell the flowers
        } else {
            // Individual poll task wake-up.
            process_event_execute(static_cast<silicon::scheduler::poll_info *>(handle_ptr), poll_status);
        }
    }

    // Its important to not resume any handles until the full set is accounted for.  If a timeout
    // and an event for the same handle happen in the same epoll_wait() call then inline processing
    // will destruct the poll_info object before the second event is handled.  This is also possible
    // with thread pool processing, but probably has an extremely low chance of occuring due to
    // the thread switch required.  If m_max_events == 1 this would be unnecessary.

    if(!m_p->m_handles_to_resume.empty()) {
        if(m_p->m_opts.execution_strategy == execution_strategy_t::process_tasks_inline) {
            std::size_t resumed{0};
            for(auto &handle: m_p->m_handles_to_resume) {
                handle.resume();
                ++resumed;
            }
            if(resumed > 0) {
                m_p->m_size.fetch_sub(resumed, std::memory_order::release);
            }
        } else {
            m_p->m_thread_pool->resume(m_p->m_handles_to_resume);
            m_p->m_size.fetch_sub(m_p->m_handles_to_resume.size(), std::memory_order::release);
        }

        m_p->m_handles_to_resume.clear();
    }
}

auto io_scheduler::process_scheduled_execute_inline() -> void {
    // Clear the notification by reading until the pipe is cleared, this is done before
    // resetting the flag that writes to the pipe need to happen.
    while(true) {
        constexpr std::size_t READ_COUNT{4};
        constexpr long READ_COUNT_BYTES = READ_COUNT * sizeof(int);
        std::array<int, READ_COUNT> control{};
        const long read_bytes = m_p->m_schedule_pipe.read(control.data(), READ_COUNT_BYTES);
        if(read_bytes == READ_COUNT_BYTES) {
            continue;
        }

        // If we got nothing, or we got a partial read break the loop since the pipe is empty.
        if(read_bytes >= 0) {
            break;
        }

        // pipe is set to O_NONBLOCK so ignore empty blocking reads.
        if(errno == EAGAIN) {
            break;
        }

        // Not much we can do here, we're in a very bad state, lets report to stderr.
        std::cerr << "::read(m_schedule_pipe.read_fd()) error[" << errno << "] " << std::system_category().message(errno) << " fd=["
                  << m_p->m_schedule_pipe.read_fd() << "]" << std::endl;
        break;
    }

    // Note to all producers that the pipe is cleared and any new additions need to trigger the pipe.
    m_p->m_schedule_pipe_triggered.exchange(false, std::memory_order::release);

    // Now it is safe to acquire all scheduled ops.
    auto *ops = silicon::coroutine::awaiter_list_pop_all(m_p->m_scheduled_ops);

    if(ops != nullptr) {
        ops = silicon::coroutine::awaiter_list_reverse(ops);

        while(ops != nullptr) {
            auto *next = ops->m_next;
            m_p->m_handles_to_resume.emplace_back(ops->m_awaiting_coroutine);

            if(ops->m_allocated) {
                delete ops;
            }

            ops = next;
        }
    }
}

auto io_scheduler::process_event_execute(silicon::scheduler::poll_info *pi, poll_status status) -> void {
    if(!pi->m_p->m_processed) {
        std::atomic_thread_fence(std::memory_order::acquire);
        // Its possible the event and the timeout occurred in the same epoll, make sure only one
        // is ever processed, the other is discarded.
        pi->m_p->m_processed = true;

        // Given a valid fd always remove it from epoll so the next poll can blindly EPOLL_CTL_ADD.
        if(pi->m_p->m_fd != -1) {
            m_p->m_io_notifier.unwatch(*pi);
        }

        // Since this event triggered, remove its corresponding timeout if it has one.
        if(pi->m_p->m_timer_pos.has_value()) {
            remove_timer_token(pi->m_p->m_timer_pos.value());
        }

        pi->m_p->m_poll_status = status;

        while(pi->m_p->m_awaiting_coroutine == nullptr) {
            std::atomic_thread_fence(std::memory_order::acquire);
        }

        m_p->m_handles_to_resume.emplace_back(pi->m_p->m_awaiting_coroutine);
    }
}

auto io_scheduler::process_timeout_execute() -> void {
    std::vector<silicon::scheduler::poll_info *> poll_infos{};
    auto now = clock::now();

    {
        std::scoped_lock lk{m_p->m_timed_events_mutex};
        while(!m_p->m_timed_events.empty()) {
            auto first = m_p->m_timed_events.begin();
            auto [tp, pi] = *first;

            if(tp <= now) {
                m_p->m_timed_events.erase(first);
                poll_infos.emplace_back(pi);
            } else {
                break;
            }
        }
    }

    for(auto pi: poll_infos) {
        if(!pi->m_p->m_processed) {
            // Its possible the event and the timeout occurred in the same epoll, make sure only one
            // is ever processed, the other is discarded.
            pi->m_p->m_processed = true;

            // Since this timed out, remove its corresponding event if it has one.
            if(pi->m_p->m_fd != -1) {
                m_p->m_io_notifier.unwatch(*pi);
            }

            while(pi->m_p->m_awaiting_coroutine == nullptr) {
                std::atomic_thread_fence(std::memory_order::acquire);
            }

            m_p->m_handles_to_resume.emplace_back(pi->m_p->m_awaiting_coroutine);
            pi->m_p->m_poll_status = silicon::coroutine::poll_status::timeout;
        }
    }

    // Update the time to the next smallest time point, re-take the current now time
    // since updating and resuming tasks could shift the time.
    update_timeout(clock::now());
}

auto io_scheduler::add_timer_token(time_point tp, silicon::scheduler::poll_info &pi) -> timed_events::iterator {
    std::scoped_lock lk{m_p->m_timed_events_mutex};
    auto pos = m_p->m_timed_events.emplace(tp, &pi);

    // If this item was inserted as the smallest time point, update the timeout.
    if(pos == m_p->m_timed_events.begin()) {
        update_timeout(clock::now());
    }

    return pos;
}

auto io_scheduler::remove_timer_token(timed_events::iterator pos) -> void {
    {
        std::scoped_lock lk{m_p->m_timed_events_mutex};
        auto is_first = (m_p->m_timed_events.begin() == pos);

        m_p->m_timed_events.erase(pos);

        // If this was the first item, update the timeout.  It would be acceptable to just let it
        // also fire the timeout as the event loop will ignore it since nothing will have timed
        // out but it feels like the right thing to do to update it to the correct timeout value.
        if(is_first) {
            update_timeout(clock::now());
        }
    }
}

auto io_scheduler::update_timeout(time_point now) -> void {
    if(!m_p->m_timed_events.empty()) {
        auto &[tp, pi] = *m_p->m_timed_events.begin();

        auto amount = tp - now;

        if(!m_p->m_io_notifier.watch_timer(m_p->m_timer, amount)) {
#if defined(_WIN32)
            std::cerr << "Failed to set timer, error=[" << GetLastError() << "].";
#else
            std::cerr << "Failed to set timerfd errorno=[" << std::system_category().message(errno) << "].";
#endif
        }
    } else {
        m_p->m_io_notifier.unwatch_timer(m_p->m_timer);
    }
}

} // namespace silicon::scheduler

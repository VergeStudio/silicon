module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    include <Windows.h>
#else
#    include <sys/socket.h>
#    include <sys/types.h>
#    include <unistd.h>
#endif

#include <array>
#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <cstring>
#include <exception>
#include <functional>
#include <system_error>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
#include <expected>
#include <map>

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :poll_info_impl;

using namespace std::chrono_literals;

using namespace silicon::scheduler;

namespace silicon::scheduler {

static silicon::scheduler::task<void> make_spawned_joinable_wait_task(std::unique_ptr<silicon::scheduler::task_group<silicon::scheduler::io_scheduler>> group_ptr) {
    co_await *group_ptr;
    co_return;
}

io_scheduler::io_scheduler(options &&opts, private_constructor)
    : m_p(std::make_unique<impl>(std::move(opts))) {

    m_p->m_recent_events.reserve(m_max_events);
}

std::expected<std::unique_ptr<io_scheduler>, std::error_code> io_scheduler::create(options opts) {
    auto s = std::make_unique<io_scheduler>(std::move(opts), private_constructor{});

    if(!s->m_p->m_shutdown_pipe.is_valid() || !s->m_p->m_schedule_pipe.is_valid()) {
        return std::unexpected(make_error_code(scheduler_error::kPipeCreateFailed));
    }
    if(!s->m_p->m_io_notifier.is_valid()) {
        return std::unexpected(make_error_code(scheduler_error::kInvalidNotifierState));
    }

    if(!s->m_p->m_io_notifier.watch(s->m_p->m_shutdown_pipe.read_fd(), silicon::scheduler::poll_op::read, const_cast<void *>(s->m_p->m_shutdown_ptr), true)) {
        return std::unexpected(make_error_code(scheduler_error::kEventRegisterFailed));
    }
    if(!s->m_p->m_io_notifier.watch(s->m_p->m_schedule_pipe.read_fd(), silicon::scheduler::poll_op::read, const_cast<void *>(s->m_p->m_schedule_ptr), true)) {
        return std::unexpected(make_error_code(scheduler_error::kEventRegisterFailed));
    }

    if(s->m_p->m_opts.execution_strategy == execution_strategy_t::process_tasks_on_thread_pool) {
        auto tp = thread_pool::create(std::move(s->m_p->m_opts.pool));
        if(!tp) {
            return std::unexpected(tp.error());
        }
        s->m_p->m_thread_pool = std::move(*tp);
    }

    if(s->m_p->m_opts.thread_strategy == thread_strategy_t::spawn) {
        try {
            s->m_p->m_io_thread = std::thread([s = s.get()]() { s->process_events_dedicated_thread(); });
        } catch(const std::exception &) {
            return std::unexpected(make_error_code(scheduler_error::kUnknown));
        }
    }

    return s;
}

io_scheduler::~io_scheduler() {
    shutdown();

    if(m_p->m_io_thread.joinable()) {
        m_p->m_io_thread.join();
    }

    destroy_completion_engine();

    m_p->m_shutdown_pipe.close();
    m_p->m_schedule_pipe.close();
}

std::size_t io_scheduler::process_events(std::chrono::milliseconds timeout) {
    process_events_manual(timeout);
    return size();
}

bool io_scheduler::spawn_detached(silicon::scheduler::task<void> &&task) {
    m_p->m_size.fetch_add(1, std::memory_order::release);
    auto wrapper_task = silicon::scheduler::make_task_self_deleting(std::move(task));
    wrapper_task.promise().user_final_suspend([this]() -> void { m_p->m_size.fetch_sub(1, std::memory_order::release); });
    return resume(wrapper_task.handle());
}

silicon::scheduler::task<void> io_scheduler::spawn_joinable(silicon::scheduler::task<void> &&task) {
    auto group_ptr = std::make_unique<silicon::scheduler::task_group<silicon::scheduler::io_scheduler>>(this, std::move(task));
    return make_spawned_joinable_wait_task(std::move(group_ptr));
}

silicon::scheduler::task<void> io_scheduler::schedule_at(time_point time) {
    return yield_until(time);
}

silicon::scheduler::task<void> io_scheduler::yield_until(time_point time) {
    auto now = clock::now();

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

silicon::scheduler::task<poll_status> io_scheduler::poll(
        fd_t fd,
        silicon::scheduler::poll_op op,
        std::chrono::milliseconds timeout,
        std::optional<poll_stop_token> cancel_trigger
) {

    m_p->m_size.fetch_add(1, std::memory_order::release);

    bool timeout_requested = (timeout > 0ms);

    auto pi = silicon::scheduler::poll_info{fd, op, cancel_trigger};

    if(timeout_requested) {
        pi.m_p->m_timer_pos = add_timer_token(clock::now() + timeout, pi);
    }

    if(!m_p->m_io_notifier.watch(pi)) {
        std::cerr << "Failed to add " << fd << " to watch list\n";
    }

    auto result = co_await pi;
    co_return result;
}

bool io_scheduler::resume(std::coroutine_handle<> handle) {
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

void io_scheduler::shutdown() noexcept {

    if(m_p->m_shutdown_requested.exchange(true, std::memory_order::acq_rel) == false) {

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

silicon::scheduler::task<void> io_scheduler::yield_for_internal(std::chrono::nanoseconds amount) {
    if(amount <= 0ms) {
        co_await schedule();
    } else {

        m_p->m_size.fetch_add(1, std::memory_order::release);

        silicon::scheduler::poll_info pi{};
        add_timer_token(clock::now() + amount, pi);
        co_await pi;
    }
    co_return;
}

void io_scheduler::process_events_manual(std::chrono::milliseconds timeout) {
    bool expected{false};
    if(m_p->m_io_processing.compare_exchange_strong(expected, true, std::memory_order::release, std::memory_order::relaxed)) {
        process_events_execute(timeout);
        m_p->m_io_processing.exchange(false, std::memory_order::release);
    }
}

void io_scheduler::process_events_dedicated_thread() {
    if(m_p->m_opts.on_io_thread_start_functor != nullptr) {
        m_p->m_opts.on_io_thread_start_functor();
    }

    m_p->m_io_processing.exchange(true, std::memory_order::release);

    while(!m_p->m_shutdown_requested.load(std::memory_order::acquire) || size() > 0) {
        process_events_execute(m_default_timeout);
    }
    m_p->m_io_processing.exchange(false, std::memory_order::release);

    if(m_p->m_opts.on_io_thread_stop_functor != nullptr) {
        m_p->m_opts.on_io_thread_stop_functor();
    }
}

void io_scheduler::process_events_execute(std::chrono::milliseconds timeout) {

    m_p->m_recent_events.clear();
    m_p->m_io_notifier.next_events(m_p->m_recent_events, timeout);

    for(auto &[handle_ptr, poll_status]: m_p->m_recent_events) {
        if(handle_ptr == m_p->m_timer_ptr) {

            process_timeout_execute();
        } else if(handle_ptr == m_p->m_schedule_ptr) {

            process_scheduled_execute_inline();
        } else if(handle_ptr == m_p->m_shutdown_ptr) [[unlikely]] {

        } else if(handle_ptr == m_p->m_completion_ptr) [[unlikely]] {

            drain_ring_completions();
        } else {

            process_event_execute(static_cast<silicon::scheduler::poll_info *>(handle_ptr), poll_status);
        }
    }

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

void io_scheduler::process_scheduled_execute_inline() {

    while(true) {
        constexpr std::size_t READ_COUNT{4};
        constexpr long READ_COUNT_BYTES = READ_COUNT * sizeof(int);
        std::array<int, READ_COUNT> control{};
        const long read_bytes = m_p->m_schedule_pipe.read(control.data(), READ_COUNT_BYTES);
        if(read_bytes == READ_COUNT_BYTES) {
            continue;
        }

        if(read_bytes >= 0) {
            break;
        }

        if(errno == EAGAIN) {
            break;
        }

        std::cerr << "::read(m_schedule_pipe.read_fd()) error[" << errno << "] " << std::system_category().message(errno) << " fd=["
                  << m_p->m_schedule_pipe.read_fd() << "]" << std::endl;
        break;
    }

    m_p->m_schedule_pipe_triggered.exchange(false, std::memory_order::release);

    auto *ops = silicon::scheduler::awaiter_list_pop_all(m_p->m_scheduled_ops);

    if(ops != nullptr) {
        ops = silicon::scheduler::awaiter_list_reverse(ops);

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

void io_scheduler::process_event_execute(silicon::scheduler::poll_info *pi, poll_status status) {
    if(!pi->m_p->m_processed) {
        std::atomic_thread_fence(std::memory_order::acquire);

        pi->m_p->m_processed = true;

        if(pi->m_p->m_fd != -1) {
            m_p->m_io_notifier.unwatch(*pi);
        }

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

void io_scheduler::process_timeout_execute() {
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

            pi->m_p->m_processed = true;

            if(pi->m_p->m_fd != -1) {
                m_p->m_io_notifier.unwatch(*pi);
            }

            while(pi->m_p->m_awaiting_coroutine == nullptr) {
                std::atomic_thread_fence(std::memory_order::acquire);
            }

            m_p->m_handles_to_resume.emplace_back(pi->m_p->m_awaiting_coroutine);
            pi->m_p->m_poll_status = silicon::scheduler::poll_status::timeout;
        }
    }

    update_timeout(clock::now());
}

auto io_scheduler::add_timer_token(time_point tp, silicon::scheduler::poll_info &pi) -> timed_events::iterator {
    std::scoped_lock lk{m_p->m_timed_events_mutex};
    auto pos = m_p->m_timed_events.emplace(tp, &pi);

    if(pos == m_p->m_timed_events.begin()) {
        update_timeout(clock::now());
    }

    return pos;
}

void io_scheduler::remove_timer_token(timed_events::iterator pos) {
    {
        std::scoped_lock lk{m_p->m_timed_events_mutex};
        auto is_first = (m_p->m_timed_events.begin() == pos);

        m_p->m_timed_events.erase(pos);

        if(is_first) {
            update_timeout(clock::now());
        }
    }
}

void io_scheduler::update_timeout(time_point now) {
    if(!m_p->m_timed_events.empty()) {
        auto &[tp, pi] = *m_p->m_timed_events.begin();

        auto amount = tp - now;

        if(!m_p->m_io_notifier.watch_timer(m_p->m_timer, amount)) {
#if defined(SILICON_PLATFORM_WINDOWS)
            std::cerr << "Failed to set timer, error=[" << GetLastError() << "].";
#else
            std::cerr << "Failed to set timerfd errorno=[" << std::system_category().message(errno) << "].";
#endif
        }
    } else {
        m_p->m_io_notifier.unwatch_timer(m_p->m_timer);
    }
}

}

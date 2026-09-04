module;

#include <atomic>
#include <variant>
#include <string>
#include <mutex>
#include <utility>

#include <coroutine>

#if !defined(SILICON_PLATFORM_WINDOWS)
#    include <unistd.h>
#endif

#include <chrono>
#include <cstdio>
#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <vector>

#include <expected>
#include <system_error>

#include <silicon/common.h>
export module silicon.scheduler:io_scheduler;
export import silicon.scheduler.error;

import silicon.error;

import :concepts.range_of;
import :awaiter_list;
import :pipe;
import :expected;
import :fd;
import :poll;
import :sync_wait;
import :time;
import silicon.scheduler.task;

import :facade;
import :thread_pool;

import :poll_info;
import :io_notifier;
import :io_ring;
import :timer_handle;

using namespace silicon::scheduler;

export namespace silicon::scheduler {

template<typename T>
using result = silicon::error::result<T>;

enum class timeout_status {
    kNoTimeout,
    kTimeout,
};

class CORE_API io_scheduler {
    using timed_events = silicon::scheduler::poll_info::timed_events;

    struct private_constructor {
        explicit private_constructor() = default;
    };

  public:
    class schedule_operation;
    friend schedule_operation;

    enum class thread_strategy_t {

        spawn,

        manual
    };

    enum class execution_strategy_t {

        process_tasks_on_thread_pool,

        process_tasks_inline
    };

    enum class io_completion_policy {

        disabled,

        enabled,

        auto_
    };

    struct options {

        thread_strategy_t thread_strategy{thread_strategy_t::spawn};

        std::function<void()> on_io_thread_start_functor{nullptr};

        std::function<void()> on_io_thread_stop_functor{nullptr};

        thread_pool::options pool{
                .thread_count = ((std::thread::hardware_concurrency() > 1) ? (std::thread::hardware_concurrency() - 1) : 1),
                .on_thread_start_functor = nullptr,
                .on_thread_stop_functor = nullptr
        };

        execution_strategy_t execution_strategy{execution_strategy_t::process_tasks_on_thread_pool};

        io_completion_policy completion_policy{
#if defined(SILICON_FEATURE_IO_RING)
                io_completion_policy::auto_
#else
                io_completion_policy::disabled
#endif
        };

        io_ring_config io_ring_cfg{};
    };

    explicit io_scheduler(options &&opts, private_constructor);

    static auto create(
            options = options{
                    .thread_strategy = thread_strategy_t::spawn,
                    .on_io_thread_start_functor = nullptr,
                    .on_io_thread_stop_functor = nullptr,
                    .pool =
                            {.thread_count =
                                     ((std::thread::hardware_concurrency() > 1) ? (std::thread::hardware_concurrency() - 1) : 1),
                             .on_thread_start_functor = nullptr,
                             .on_thread_stop_functor = nullptr},
                    .execution_strategy = execution_strategy_t::process_tasks_on_thread_pool,

#if defined(SILICON_FEATURE_IO_RING)
                    .completion_policy = io_completion_policy::auto_,
#else
                    .completion_policy = io_completion_policy::disabled,
#endif
                    .io_ring_cfg = {}
            }
    ) -> result<std::unique_ptr<io_scheduler>>;

    io_scheduler(const io_scheduler &) = delete;
    io_scheduler(io_scheduler &&) = delete;
    io_scheduler & operator=(const io_scheduler &) = delete;
    io_scheduler & operator=(io_scheduler &&) = delete;

    ~io_scheduler();

    auto process_events(std::chrono::milliseconds = std::chrono::milliseconds{0}) -> std::size_t;

    class schedule_operation {
        friend class io_scheduler;
        explicit schedule_operation(io_scheduler &scheduler) noexcept: m_scheduler(scheduler) {}

      public:

        bool await_ready() noexcept { return false; }

        void await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
            if(m_scheduler.m_p->m_opts.execution_strategy == execution_strategy_t::process_tasks_inline) {
                m_scheduler.m_p->m_size.fetch_add(1, std::memory_order::release);
                m_awaiting_coroutine = awaiting_coroutine;
                silicon::scheduler::awaiter_list_push(m_scheduler.m_p->m_scheduled_ops, this);

                bool expected{false};
                if(m_scheduler.m_p->m_schedule_pipe_triggered.compare_exchange_strong(
                           expected, true, std::memory_order::release, std::memory_order::relaxed
                   )) {
                    constexpr int control = 1;
                    long written = m_scheduler.m_p->m_schedule_pipe.write(&control, sizeof(control));
                    if(written != sizeof(control)) {
                        std::string error_msg = std::format(
                            "silicon::scheduler::io_scheduler::schedule_operation failed to write to schedule pipe, bytes written={}\n",
                            written);
                        std::fputs(error_msg.c_str(), stderr);
                    }
                }
            } else {
                m_scheduler.m_p->m_thread_pool->resume(awaiting_coroutine);
            }
        }

        void await_resume() noexcept {}

        std::coroutine_handle<> m_awaiting_coroutine;
        schedule_operation *m_next{nullptr};
        bool m_allocated{false};

      private:

        io_scheduler &m_scheduler;
    };

    schedule_operation schedule() { return schedule_operation{*this}; }

    bool spawn_detached(silicon::scheduler::task<void> &&task) ;

    silicon::scheduler::task<void> spawn_joinable(silicon::scheduler::task<void> &&task) ;

    template<typename return_type>
    [[nodiscard]] silicon::scheduler::task<return_type> schedule(silicon::scheduler::task<return_type> task) {
        co_await schedule();
        co_return co_await task;
    }

    template<typename return_type, typename rep, typename period>
    [[nodiscard]] silicon::scheduler::task<silicon::scheduler::expected<return_type, timeout_status>> schedule(silicon::scheduler::task<return_type> task, std::chrono::duration<rep, period> timeout) {
        using namespace std::chrono_literals;

        auto timeout_ms = std::max(std::chrono::duration_cast<std::chrono::milliseconds>(timeout), 0ms);
        if(timeout_ms == 0ms) {
            if constexpr(std::is_void_v<return_type>) {
                co_await schedule(std::move(task));
                co_return silicon::scheduler::expected<return_type, timeout_status>();
            } else {
                co_return silicon::scheduler::expected<return_type, timeout_status>(co_await schedule(std::move(task)));
            }
        }

        auto result = co_await when_any(std::move(task), make_timeout_task(timeout_ms));
        if(!std::holds_alternative<timeout_status>(result)) {
            if constexpr(std::is_void_v<return_type>) {
                co_return silicon::scheduler::expected<return_type, timeout_status>();
            } else {
                co_return silicon::scheduler::expected<return_type, timeout_status>(std::move(std::get<0>(result)));
            }
        } else {
            co_return silicon::scheduler::unexpected<timeout_status>(std::move(std::get<1>(result)));
        }
    }

#ifndef EMSCRIPTEN

    template<typename return_type, typename rep, typename period>
    [[nodiscard]] silicon::scheduler::task<silicon::scheduler::expected<return_type, timeout_status>> schedule(std::stop_source stop_source, silicon::scheduler::task<return_type> task, std::chrono::duration<rep, period> timeout) {
        using namespace std::chrono_literals;

        auto timeout_ms = std::max(std::chrono::duration_cast<std::chrono::milliseconds>(timeout), 0ms);
        if(timeout_ms == 0ms) {
            if constexpr(std::is_void_v<return_type>) {
                co_await schedule(std::move(task));
                co_return silicon::scheduler::expected<return_type, timeout_status>();
            } else {
                co_return silicon::scheduler::expected<return_type, timeout_status>(co_await schedule(std::move(task)));
            }
        }

        auto result = co_await when_any(std::move(stop_source), std::move(task), make_timeout_task(timeout_ms));
        if(!std::holds_alternative<timeout_status>(result)) {
            if constexpr(std::is_void_v<return_type>) {
                co_return silicon::scheduler::expected<return_type, timeout_status>();
            } else {
                co_return silicon::scheduler::expected<return_type, timeout_status>(std::move(std::get<0>(result)));
            }
        } else {
            co_return silicon::scheduler::unexpected<timeout_status>(std::move(std::get<1>(result)));
        }
    }
#endif

    template<class rep_type, class period_type>
    [[nodiscard]] silicon::scheduler::task<void> schedule_after(std::chrono::duration<rep_type, period_type> amount) {
        return yield_for_internal(std::chrono::duration_cast<std::chrono::nanoseconds>(amount));
    }

    [[nodiscard]] silicon::scheduler::task<void> schedule_at(time_point) ;

    [[nodiscard]] schedule_operation yield() { return schedule_operation{*this}; };

    template<class rep_type, class period_type>
    [[nodiscard]] silicon::scheduler::task<void> yield_for(std::chrono::duration<rep_type, period_type> amount) {
        return yield_for_internal(std::chrono::duration_cast<std::chrono::nanoseconds>(amount));
    }

    [[nodiscard]] silicon::scheduler::task<void> yield_until(time_point) ;

    [[nodiscard]] auto poll(
            fd_t,
            silicon::scheduler::poll_op,
            std::chrono::milliseconds = std::chrono::milliseconds{0},
            std::optional<poll_stop_token> = std::nullopt
    ) -> silicon::scheduler::task<poll_status>;

    [[nodiscard]] silicon::scheduler::task<result<int64_t>> read_at(
            fd_t, void *, std::uint32_t, std::uint64_t) ;

    [[nodiscard]] silicon::scheduler::task<result<int64_t>> write_at(
            fd_t, const void *, std::uint32_t, std::uint64_t) ;

    bool resume(std::coroutine_handle<>) ;

    template<silicon::scheduler::concepts::sized_range_of<std::coroutine_handle<>> range_type>
    std::size_t resume(const range_type &handles) noexcept {
        auto size = std::size(handles);
        std::size_t invalid_handles{0};
        for(const auto &handle: handles) {
            if(!resume(handle)) {
                ++invalid_handles;
            }
        }

        return size - invalid_handles;
    }

    std::size_t size() const noexcept {
        if(m_p->m_opts.execution_strategy == execution_strategy_t::process_tasks_inline) {
            return m_p->m_size.load(std::memory_order::acquire);
        } else {
            return m_p->m_size.load(std::memory_order::acquire) + m_p->m_thread_pool->size();
        }
    }

    bool empty() const noexcept { return size() == 0; }

    void shutdown() noexcept ;

    [[nodiscard]] bool is_shutdown() const { return m_p->m_shutdown_requested.load(std::memory_order::acquire); }

    silicon::scheduler::io_notifier & io_notifier() { return m_p->m_io_notifier; }

    [[nodiscard]] io_ring::backend completion_backend() const noexcept ;

  private:
    struct impl {
      public:
        explicit impl(options &&opts)
            : m_opts(std::move(opts)),
              m_io_notifier(),
              m_shutdown_poll(),
              m_schedule_poll(),
              m_timer_poll(),
              m_timer(&m_timer_poll, m_io_notifier),
              m_shutdown_ptr(&m_shutdown_poll),
              m_schedule_ptr(&m_schedule_poll),
              m_timer_ptr(&m_timer_poll) {}

        options m_opts;

        ::silicon::scheduler::io_notifier m_io_notifier;

        silicon::scheduler::poll_info m_shutdown_poll{};
        silicon::scheduler::poll_info m_schedule_poll{};
        silicon::scheduler::poll_info m_timer_poll{};

        silicon::scheduler::timer_handle m_timer;

        void *m_shutdown_ptr = &m_shutdown_poll;
        void *m_schedule_ptr = &m_schedule_poll;
        void *m_timer_ptr = &m_timer_poll;

        silicon::scheduler::poll_info m_completion_poll{};
        void *m_completion_ptr = &m_completion_poll;

        std::once_flag m_completion_once{};

        void *m_completion_engine{nullptr};

        silicon::scheduler::pipe_t m_shutdown_pipe{};

        silicon::scheduler::pipe_t m_schedule_pipe{};

        std::atomic<bool> m_schedule_pipe_triggered{false};

        std::atomic<schedule_operation *> m_scheduled_ops{nullptr};

        std::atomic<std::size_t> m_size{0};

        std::thread m_io_thread;

        std::unique_ptr<thread_pool> m_thread_pool{nullptr};

        std::mutex m_timed_events_mutex{};

        timed_events m_timed_events{};

        std::atomic<bool> m_shutdown_requested{false};

        std::atomic<bool> m_io_processing{false};

        std::vector<std::pair<silicon::scheduler::poll_info *, silicon::scheduler::poll_status>> m_recent_events{};
        std::vector<std::coroutine_handle<>> m_handles_to_resume{};
    };

    std::unique_ptr<impl> m_p;

    static const constexpr std::chrono::milliseconds m_default_timeout{1000};
    static const constexpr std::chrono::milliseconds m_no_timeout{0};
    static const constexpr std::size_t m_max_events = 16;

    silicon::scheduler::task<void> yield_for_internal(std::chrono::nanoseconds) ;
    void process_events_manual(std::chrono::milliseconds) ;
    void process_events_dedicated_thread() ;
    void process_events_execute(std::chrono::milliseconds) ;
    static poll_status event_to_poll_status(uint32_t) ;

    void process_scheduled_execute_inline() ;

    void process_event_execute(silicon::scheduler::poll_info *, poll_status) ;
    void process_timeout_execute() ;

    void drain_ring_completions() ;

    void destroy_completion_engine() ;

    auto add_timer_token(time_point, silicon::scheduler::poll_info &) -> timed_events::iterator;
    void remove_timer_token(timed_events::iterator) ;
    void update_timeout(time_point) ;

    silicon::scheduler::task<timeout_status> make_timeout_task(std::chrono::milliseconds timeout) {
        co_await schedule_after(timeout);
        co_return timeout_status::kTimeout;
    }

};

}

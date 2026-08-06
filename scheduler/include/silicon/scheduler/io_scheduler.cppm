module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <atomic>
#include <variant> // std::holds_alternative
#include <string>
#include <mutex>
#include <utility>


#include <coroutine>



#ifdef LIBCORO_FEATURE_NETWORKING
#include "silicon/network/socket.hpp"
#endif

#if !defined(_WIN32)
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

export module silicon.scheduler:io_scheduler;

// io_scheduler 原为 silicon.coroutine:scheduler。迁出后不能再用分区 import
// （import :x 仅限同模块单元），改为整体导入 silicon.coroutine。
import silicon.coroutine;
import silicon.scheduler.task;

import :ischeduler;
import :thread_pool;

// 本单元沿用 coroutine 的基础类型（fd_t / poll_op / poll_status / poll_stop_token /
// time_point / when_any / expected ...）。using-directive 置于全局作用域：命名空间内
// 的同名实体优先，不会与 silicon::scheduler::task 冲突；且它不参与模块导出。
using namespace silicon::coroutine;

export namespace silicon::scheduler {

enum class timeout_status {
    kNoTimeout,
    kTimeout,
};

class io_scheduler: public IScheduler {
    using timed_events = silicon::coroutine::detail::poll_info::timed_events;

    struct private_constructor {
        explicit private_constructor() = default;
    };

  public:
    class schedule_operation;
    friend schedule_operation;

    enum class thread_strategy_t {
        /// Spawns a dedicated background thread for the scheduler to run on.
        spawn,
        /// Requires the user to call process_events() to drive the scheduler.
        manual
    };

    enum class execution_strategy_t {
        /// Tasks will be FIFO queued to be executed on a thread pool.  This is better for tasks that
        /// are long lived and will use lots of CPU because long lived tasks will block other i/o
        /// operations while they complete.  This strategy is generally better for lower latency
        /// requirements at the cost of throughput.
        process_tasks_on_thread_pool,
        /// Tasks will be executed inline on the io scheduler thread.  This is better for short tasks
        /// that can be quickly processed and not block other i/o operations for very long.  This
        /// strategy is generally better for higher throughput at the cost of latency.
        process_tasks_inline
    };

    struct options {
        /// Should the io scheduler spawn a dedicated event processor?
        thread_strategy_t thread_strategy{thread_strategy_t::spawn};
        /// If spawning a dedicated event processor a functor to call upon that thread starting.
        std::function<void()> on_io_thread_start_functor{nullptr};
        /// If spawning a dedicated event processor a functor to call upon that thread stopping.
        std::function<void()> on_io_thread_stop_functor{nullptr};
        /// Thread pool options for the task processor threads.  See thread pool for more details.
        thread_pool::options pool{
                .thread_count = ((std::thread::hardware_concurrency() > 1) ? (std::thread::hardware_concurrency() - 1) : 1),
                .on_thread_start_functor = nullptr,
                .on_thread_stop_functor = nullptr
        };

        /// If inline task processing is enabled then the io worker will resume tasks on its thread
        /// rather than scheduling them to be picked up by the thread pool.
        execution_strategy_t execution_strategy{execution_strategy_t::process_tasks_on_thread_pool};
    };

    /**
     * @see io_scheduler::make_unique
     */
    explicit io_scheduler(options &&opts, private_constructor);

    /**
     * @brief Creates an io_scheduler executor.
     *
     * @param opts The scheduler's options.
     * @return std::unique_ptr<io_scheduler>
     */
    static auto make_unique(
            options opts = options{
                    .thread_strategy = thread_strategy_t::spawn,
                    .on_io_thread_start_functor = nullptr,
                    .on_io_thread_stop_functor = nullptr,
                    .pool =
                            {.thread_count =
                                     ((std::thread::hardware_concurrency() > 1) ? (std::thread::hardware_concurrency() - 1) : 1),
                             .on_thread_start_functor = nullptr,
                             .on_thread_stop_functor = nullptr},
                    .execution_strategy = execution_strategy_t::process_tasks_on_thread_pool
            }
    ) -> std::unique_ptr<io_scheduler>;

    io_scheduler(const io_scheduler &) = delete;
    io_scheduler(io_scheduler &&) = delete;
    auto operator=(const io_scheduler &) -> io_scheduler & = delete;
    auto operator=(io_scheduler &&) -> io_scheduler & = delete;

    ~io_scheduler() override;

    /**
     * Given a thread_strategy_t::manual this function should be called at regular intervals to
     * process events that are ready.  If a using thread_strategy_t::spawn this is run continously
     * on a dedicated background thread and does not need to be manually invoked.
     * @param timeout If no events are ready how long should the function wait for events to be ready?
     *                Passing zero (default) for the timeout will check for any events that are
     *                ready now, and then return.  This could be zero events.  Passing -1 means block
     *                indefinitely until an event happens.
     * @return The number of tasks currently executing or waiting to execute.
     */
    auto process_events(std::chrono::milliseconds timeout = std::chrono::milliseconds{0}) -> std::size_t;

    class schedule_operation {
        friend class io_scheduler;
        explicit schedule_operation(io_scheduler &scheduler) noexcept: m_scheduler(scheduler) {}

      public:
        /**
         * Operations always pause so the executing thread can be switched.
         */
        auto await_ready() noexcept -> bool { return false; }

        /**
         * Suspending always returns to the caller (using void return of await_suspend()) and
         * stores the coroutine internally for the executing thread to resume from.
         */
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> void {
            if(m_scheduler.m_p->m_opts.execution_strategy == execution_strategy_t::process_tasks_inline) {
                m_scheduler.m_p->m_size.fetch_add(1, std::memory_order::release);
                m_awaiting_coroutine = awaiting_coroutine;
                silicon::coroutine::detail::awaiter_list_push(m_scheduler.m_p->m_scheduled_ops, this);

                // Trigger the event to wake-up the scheduler if this event isn't currently triggered.
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

        /**
         * no-op as this is the function called first by the thread pool's executing thread.
         */
        auto await_resume() noexcept -> void {}

        std::coroutine_handle<> m_awaiting_coroutine;
        schedule_operation *m_next{nullptr};
        bool m_allocated{false};

      private:
        /// The thread pool that this operation will execute on.
        io_scheduler &m_scheduler;
    };

    /**
     * Schedules the current task onto this scheduler for execution.
     */
    auto schedule() -> schedule_operation { return schedule_operation{*this}; }

    /**
     * Spawns a task into the scheduler and moves ownership of the task to the scheduler.
     * Only void return type tasks can be spawned in this manner since the task submitter will no
     * longer have control over the spawned task, it is effectively detached.
     * @param task The task to execute on this scheduler.  It's lifetime ownership will be transferred
     *             to this scheduler.
     * @return True if the task was succesfully spawned onto the scheduler. This can fail if the task
     *         is already completed or does not contain a valid coroutine anymore.
     */
    auto spawn_detached(silicon::coroutine::task<void> &&task) -> bool override;

    /**
     * Spawns the given task to be run on this scheduler, the task returned must be joined in the future.
     * @note The returned task shouldn't be co_await'ed immediately, the spawned task is started on the scheduler
     *       automatically but the returned join task *must* be co_await'ed at some point in the future.
     *       If you drop the returned task it will hang the thread until the spawned task completes so it is
     *       highly advisable to co_await the returned join task appropriately.
     * @param task The task to spawn onto the scheduler.
     * @return A task that can be co_await'ed (joined) in the future to know when the spawned task is complete.
     */
    auto spawn_joinable(silicon::coroutine::task<void> &&task) -> silicon::coroutine::task<void> override;

    /**
     * Schedules a task on the scheduler and returns another task that must be awaited on for completion.
     * This can be done via co_await in a coroutine context or silicon::coroutine::sync_wait() outside of coroutine context.
     * @tparam return_type The return value of the task.
     * @param task The task to schedule on the scheduler.
     * @return The task to await for the input task to complete.
     */
    template<typename return_type>
    [[nodiscard]] auto schedule(silicon::coroutine::task<return_type> task) -> silicon::coroutine::task<return_type> {
        co_await schedule();
        co_return co_await task;
    }

    /**
     * Schedules a task on the scheduler that must complete within the given timeout.
     * NOTE: This version of schedule does *NOT* cancel the given task, it will continue executing even if it times
     * out. It is absolutely recommended to use the version of this schedule() function that takes an
     * std::stop_token and have the scheduled task check to see if its been cancelled due to timeout to not waste
     * resources.
     * @tparam return_type The return value of the task.
     * @param task The task to schedule on the scheduler with the given timeout.
     * @param timeout How long should this task be given to complete before it times out?
     * @return The task to await for the input task to complete.
     */
    template<typename return_type, typename rep, typename period>
    [[nodiscard]] auto schedule(silicon::coroutine::task<return_type> task, std::chrono::duration<rep, period> timeout)
            -> silicon::coroutine::task<silicon::coroutine::expected<return_type, timeout_status>> {
        using namespace std::chrono_literals;

        // If negative or 0 timeout, just schedule the task as normal.
        auto timeout_ms = std::max(std::chrono::duration_cast<std::chrono::milliseconds>(timeout), 0ms);
        if(timeout_ms == 0ms) {
            if constexpr(std::is_void_v<return_type>) {
                co_await schedule(std::move(task));
                co_return silicon::coroutine::expected<return_type, timeout_status>();
            } else {
                co_return silicon::coroutine::expected<return_type, timeout_status>(co_await schedule(std::move(task)));
            }
        }

        auto result = co_await when_any(std::move(task), make_timeout_task(timeout_ms));
        if(!std::holds_alternative<timeout_status>(result)) {
            if constexpr(std::is_void_v<return_type>) {
                co_return silicon::coroutine::expected<return_type, timeout_status>();
            } else {
                co_return silicon::coroutine::expected<return_type, timeout_status>(std::move(std::get<0>(result)));
            }
        } else {
            co_return silicon::coroutine::unexpected<timeout_status>(std::move(std::get<1>(result)));
        }
    }

#ifndef EMSCRIPTEN
    /**
     * Schedules a task on the scheduler that must complete within the given timeout.
     * NOTE: This version of the task will have the stop_source.request_stop() be called if the timeout triggers.
     *       It is up to you to check in the scheduled task if the stop has been requested to actually stop
     * executing the task.
     * @tparam return_type The return value of the task.
     * @param task The task to schedule on the scheduler with the given timeout.
     * @param timeout How long should this task be given to complete before it times out? Zero or negative timeout means
     * no timeout.
     * @return The task to await for the input task to complete.
     */
    template<typename return_type, typename rep, typename period>
    [[nodiscard]] auto
    schedule(std::stop_source stop_source, silicon::coroutine::task<return_type> task, std::chrono::duration<rep, period> timeout)
            -> silicon::coroutine::task<silicon::coroutine::expected<return_type, timeout_status>> {
        using namespace std::chrono_literals;

        // If negative or 0 timeout, just schedule the task as normal.
        auto timeout_ms = std::max(std::chrono::duration_cast<std::chrono::milliseconds>(timeout), 0ms);
        if(timeout_ms == 0ms) {
            if constexpr(std::is_void_v<return_type>) {
                co_await schedule(std::move(task));
                co_return silicon::coroutine::expected<return_type, timeout_status>();
            } else {
                co_return silicon::coroutine::expected<return_type, timeout_status>(co_await schedule(std::move(task)));
            }
        }

        auto result = co_await when_any(std::move(stop_source), std::move(task), make_timeout_task(timeout_ms));
        if(!std::holds_alternative<timeout_status>(result)) {
            if constexpr(std::is_void_v<return_type>) {
                co_return silicon::coroutine::expected<return_type, timeout_status>();
            } else {
                co_return silicon::coroutine::expected<return_type, timeout_status>(std::move(std::get<0>(result)));
            }
        } else {
            co_return silicon::coroutine::unexpected<timeout_status>(std::move(std::get<1>(result)));
        }
    }
#endif

    /**
     * Schedules the current task to run after the given amount of time has elapsed.
     * @param amount The amount of time to wait before resuming execution of this task.
     *               Given zero or negative amount of time this behaves identical to schedule().
     */
    template<class rep_type, class period_type>
    [[nodiscard]] auto schedule_after(std::chrono::duration<rep_type, period_type> amount) -> silicon::coroutine::task<void> {
        return yield_for_internal(std::chrono::duration_cast<std::chrono::nanoseconds>(amount));
    }

    /**
     * Schedules the current task to run at a given time point in the future.
     * @param time The time point to resume execution of this task.  Given 'now' or a time point
     *             in the past this behaves identical to schedule().
     */
    [[nodiscard]] auto schedule_at(time_point time) -> silicon::coroutine::task<void>;

    /**
     * Yields the current task to the end of the queue of waiting tasks.
     */
    [[nodiscard]] auto yield() -> schedule_operation { return schedule_operation{*this}; };

    /**
     * Yields the current task for the given amount of time.
     * @param amount The amount of time to yield for before resuming executino of this task.
     *               Given zero or negative amount of time this behaves identical to yield().
     */
    template<class rep_type, class period_type>
    [[nodiscard]] auto yield_for(std::chrono::duration<rep_type, period_type> amount) -> silicon::coroutine::task<void> {
        return yield_for_internal(std::chrono::duration_cast<std::chrono::nanoseconds>(amount));
    }

    /**
     * Yields the current task until the given time point in the future.
     * @param time The time point to resume execution of this task.  Given 'now' or a time point in the
     *             in the past this behaves identical to yield().
     */
    [[nodiscard]] auto yield_until(time_point time) -> silicon::coroutine::task<void>;

    /**
     * Polls the given file descriptor for the given operations.
     * @param fd The file descriptor to poll for events.
     * @param op The operations to poll for.
     * @param timeout The amount of time to wait for the events to trigger.  A timeout of zero will
     *                block indefinitely until the event triggers.
     * @return The result of the poll operation.
     */
    [[nodiscard]] auto poll(
            fd_t fd,
            silicon::coroutine::poll_op op,
            std::chrono::milliseconds timeout = std::chrono::milliseconds{0},
            std::optional<poll_stop_token> cancel_trigger = std::nullopt
    ) -> silicon::coroutine::task<poll_status>;

#ifdef LIBCORO_FEATURE_NETWORKING
    /**
     * Polls the given silicon::network::socket for the given operations.
     * @param sock The socket to poll for events on.
     * @param op The operations to poll for.
     * @param timeout The amount of time to wait for the events to trigger.  A timeout of zero will
     *                block indefinitely until the event triggers.
     * @return THe result of the poll operation.
     */
    [[nodiscard]] auto poll(
            const silicon::network::socket &sock,
            silicon::coroutine::poll_op op,
            std::chrono::milliseconds timeout = std::chrono::milliseconds{0},
            std::optional<poll_stop_token> cancel_trigger = std::nullopt
    ) -> silicon::coroutine::task<poll_status> {
        return poll(sock.native_handle(), op, timeout, cancel_trigger);
    }
#endif

    /**
     * Resumes execution of a direct coroutine handle on this io scheduler.
     * @param handle The coroutine handle to resume execution.
     */
    auto resume(std::coroutine_handle<> handle) -> bool override;

    template<silicon::coroutine::concepts::sized_range_of<std::coroutine_handle<>> range_type>
    auto resume(const range_type &handles) noexcept -> std::size_t {
        auto size = std::size(handles);
        std::size_t invalid_handles{0};
        for(const auto &handle: handles) {
            if(!resume(handle)) {
                ++invalid_handles;
            }
        }

        return size - invalid_handles;
    }

    /**
     * @return The number of tasks waiting in the task queue + the executing tasks.
     */
    auto size() const noexcept -> std::size_t override {
        if(m_p->m_opts.execution_strategy == execution_strategy_t::process_tasks_inline) {
            return m_p->m_size.load(std::memory_order::acquire);
        } else {
            return m_p->m_size.load(std::memory_order::acquire) + m_p->m_thread_pool->size();
        }
    }

    /**
     * @return True if the task queue is empty and zero tasks are currently executing.
     */
    auto empty() const noexcept -> bool override { return size() == 0; }

    /**
     * Starts the shutdown of the io scheduler.  All currently executing and pending tasks will complete
     * prior to shutting down.  This call is blocking and will not return until all tasks complete.
     */
    auto shutdown() noexcept -> void override;

    [[nodiscard]] auto is_shutdown() const -> bool override { return m_p->m_shutdown_requested.load(std::memory_order::acquire); }

    auto io_notifier() -> silicon::coroutine::io_notifier & { return m_p->m_io_notifier; }

  private:
    struct Impl {
      public:
        explicit Impl(options &&opts)
            : m_opts(std::move(opts)),
              m_io_notifier(),
              m_timer(static_cast<const void *>(&io_scheduler::m_timer_object), m_io_notifier) {}

        /// The configuration options.
        options m_opts;

        /// The io event notifier.
        ::silicon::coroutine::io_notifier m_io_notifier;
        /// The timer handle for timed events, e.g. yield_for() or scheduler_after().
        silicon::coroutine::detail::timer_handle m_timer;
        /// The event loop pipe to trigger a shutdown.
        silicon::coroutine::detail::pipe_t m_shutdown_pipe{};
        /// The event loop schedule task pipe.
        silicon::coroutine::detail::pipe_t m_schedule_pipe{};
        /// @brief Scheduled operations waiting tasks has entries.
        std::atomic<bool> m_schedule_pipe_triggered{false};
        /// @brief Scheduled operations waiting to be resumed.
        std::atomic<schedule_operation *> m_scheduled_ops{nullptr};

        /// The number of tasks executing or awaiting events in this io scheduler.
        std::atomic<std::size_t> m_size{0};

        /// The background io worker threads.
        std::thread m_io_thread;
        /// Thread pool for executing tasks when not in inline mode.
        std::unique_ptr<thread_pool> m_thread_pool{nullptr};

        std::mutex m_timed_events_mutex{};
        /// The map of time point's to poll infos for tasks that are yielding for a period of time
        /// or for tasks that are polling with timeouts.
        timed_events m_timed_events{};

        /// Has the scheduler been requested to shut down?
        std::atomic<bool> m_shutdown_requested{false};

        std::atomic<bool> m_io_processing{false};

        std::vector<std::pair<silicon::coroutine::detail::poll_info *, silicon::coroutine::poll_status>> m_recent_events{};
        std::vector<std::coroutine_handle<>> m_handles_to_resume{};
    };

    std::unique_ptr<Impl> m_p;

    static constexpr const int m_shutdown_object{0};
    static constexpr const void *m_shutdown_ptr = &m_shutdown_object;

    static constexpr const int m_timer_object{0};
    static constexpr const void *m_timer_ptr = &m_timer_object;

    static constexpr const int m_schedule_object{0};
    static constexpr const void *m_schedule_ptr = &m_schedule_object;

    static const constexpr std::chrono::milliseconds m_default_timeout{1000};
    static const constexpr std::chrono::milliseconds m_no_timeout{0};
    static const constexpr std::size_t m_max_events = 16;

    auto yield_for_internal(std::chrono::nanoseconds amount) -> silicon::coroutine::task<void>;
    auto process_events_manual(std::chrono::milliseconds timeout) -> void;
    auto process_events_dedicated_thread() -> void;
    auto process_events_execute(std::chrono::milliseconds timeout) -> void;
    static auto event_to_poll_status(uint32_t events) -> poll_status;

    auto process_scheduled_execute_inline() -> void;

    auto process_event_execute(silicon::coroutine::detail::poll_info *pi, poll_status status) -> void;
    auto process_timeout_execute() -> void;

    auto add_timer_token(time_point tp, silicon::coroutine::detail::poll_info &pi) -> timed_events::iterator;
    auto remove_timer_token(timed_events::iterator pos) -> void;
    auto update_timeout(time_point now) -> void;

    auto make_timeout_task(std::chrono::milliseconds timeout) -> silicon::coroutine::task<timeout_status> {
        co_await schedule_after(timeout);
        co_return timeout_status::kTimeout;
    }

};

} // namespace silicon::scheduler

module;

#include <coroutine>
#include <cstddef>
#include <memory>

export module silicon.scheduler:parallel_scheduler;

import silicon.scheduler.task;
import :ischeduler;
import :thread_pool;

export namespace silicon::scheduler {

/// @brief 系统级并行调度器（参考 stdexec::parallel_scheduler / get_parallel_scheduler()）。
///
/// 作为进程级的 "默认并行后端"，parallel_scheduler 内部 *拥有* 一个 thread_pool
/// （默认线程数 = std::thread::hardware_concurrency()），并把所有 scheduler_facade 操作
/// 委托给它。它对外呈现的就是一个并行线程池调度器，语义与 thread_pool 完全一致
/// （工作窃取、size / shutdown 等），区别仅在于它是 "系统默认并行上下文" 这一语义角色，
/// 并通过 get_parallel_scheduler() 取得进程唯一的实例。
///
/// 与 inline_scheduler（同步内联）、run_loop（单线程显式驱动）、io_scheduler（事件驱动）
/// 并列，构成 silicon 调度器家族的并行计算分支。它是把 CPU 密集型工作并行化时最常用的
/// 入口，等价于 stdexec 里 `ex::get_parallel_scheduler()` 所返回的调度器。
///
/// 满足 scheduler_facade（原 i_scheduler），因此可被 task_group / 上层组件统一驱动。
class parallel_scheduler final {
    struct impl;
    std::unique_ptr<impl> m_impl;

  public:
    parallel_scheduler();
    ~parallel_scheduler();

    parallel_scheduler(const parallel_scheduler &) = delete;
    parallel_scheduler(parallel_scheduler &&) = delete;
    auto operator=(const parallel_scheduler &) -> parallel_scheduler & = delete;
    auto operator=(parallel_scheduler &&) -> parallel_scheduler & = delete;

    /// @brief 底层线程池的线程数（parallel_scheduler 独有）。
    [[nodiscard]] auto thread_count() const noexcept -> std::size_t;

    // —— scheduler_facade ——
    auto spawn_detached(task<void> &&task) noexcept -> bool;
    auto spawn_joinable(task<void> &&task) noexcept -> task<void>;
    auto resume(std::coroutine_handle<> handle) noexcept -> bool;
    auto shutdown() noexcept -> void;
    auto is_shutdown() const -> bool;
    auto size() const noexcept -> std::size_t;
    auto empty() const noexcept -> bool { return size() == 0; }

    /// @brief 取得进程唯一的系统并行调度器（对应 stdexec::get_parallel_scheduler()）。
    ///
    /// 首次调用时懒惰构造，线程数为硬件并发数；其生命周期贯穿整个进程，析构时
    /// 会 join 掉底层线程池的所有工作线程。
    static auto get_parallel_scheduler() -> parallel_scheduler &;
};

} // namespace silicon::scheduler

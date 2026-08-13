module;

#include <coroutine>
#include <cstddef>

export module silicon.scheduler:ischeduler;

import silicon.scheduler.task;

export namespace silicon::scheduler {

/// @brief 调度器统一行为规范（抽象基类）。
///
/// silicon 中所有可执行协程的调度器（CPU 密集型的 thread_pool、事件驱动的
/// io_scheduler）都实现该接口，从而可以被 task_group / 上层组件以同一套
/// 语义驱动。
///
/// 只有 thread_pool 与 io_scheduler 的**公共**非模板行为在此声明为纯虚：
///   - 提交任务：spawn_detached / spawn_joinable
///   - 恢复协程：resume
///   - 生命周期：shutdown / is_shutdown
///   - 负载观测：size / empty
///
/// 各实现独有的能力不进入本接口：
///   - thread_pool：thread_count / queue_size / queue_empty / schedule()
///   - io_scheduler：poll / schedule_after / yield_for / process_events
///
/// 模板便捷重载（schedule<return_type>、resume<range_type>）无法虚化，仍留在
/// 各具体类中，并以此处的虚方法为实现基础。
class i_scheduler {
  public:
    i_scheduler() = default;
    i_scheduler(const i_scheduler &) = delete;
    i_scheduler(i_scheduler &&) = delete;
    auto operator=(const i_scheduler &) -> i_scheduler & = delete;
    auto operator=(i_scheduler &&) -> i_scheduler & = delete;

    virtual ~i_scheduler() = default;

    /// @brief 提交一个 void 任务（分离执行，调用方不再持有所有权）。
    /// @return 任务是否成功进入调度器。
    virtual auto spawn_detached(silicon::scheduler::task<void> &&task) -> bool = 0;

    /// @brief 提交一个 void 任务，返回可 join（co_await）的任务句柄。
    virtual auto spawn_joinable(silicon::scheduler::task<void> &&task)
            -> silicon::scheduler::task<void> = 0;

    /// @brief 在本调度器上恢复一个裸协程句柄。
    virtual auto resume(std::coroutine_handle<> handle) -> bool = 0;

    /// @brief 发起关闭；阻塞直到所有在执行 / 待执行任务完成。
    virtual auto shutdown() noexcept -> void = 0;

    /// @brief 是否已请求关闭。
    [[nodiscard]] virtual auto is_shutdown() const -> bool = 0;

    /// @brief 正在执行 + 排队中的任务数。
    [[nodiscard]] virtual auto size() const noexcept -> std::size_t = 0;

    /// @brief 是否没有任何任务在执行或排队。
    [[nodiscard]] virtual auto empty() const noexcept -> bool = 0;
};

} // namespace silicon::scheduler

module;

#include <coroutine>
#include <cstddef>
#include <memory>

export module silicon.scheduler:inline_scheduler;

import silicon.scheduler.task;
import :ischeduler;

export namespace silicon::scheduler {

/// @brief 内联调度器（参考 stdexec::inline_scheduler）。
///
/// 在当前调用线程上 *立即同步* 执行提交的工作：不做排队、不切换线程、不做动态分配。
/// 无论通过 spawn_*() 提交还是通过 resume() 恢复，协程句柄都在调用 resume() 的那个
/// 线程上就地 resume()，因此 completion 是同步发生的（对应 stdexec 的
/// __inline_completion —— 不引入任何异步性）。
///
/// 与 run_loop（单线程但显式队列 + run() 驱动）、thread_pool（多工作线程、工作窃取）、
/// io_scheduler（事件驱动）都不同，inline_scheduler *不拥有任何线程*，也不维护队列；
/// 它本质上是 "把工作直接跑在当前栈上"。典型用途：
///   - 默认 / 零开销占位调度器；
///   - 把工作同步驱动到调用方所在线程；
///   - 测试与递归场景（避免额外的调度跳转与栈外溢风险）。
///
/// 它实现 IScheduler，因此对 task_group 等上层组件而言与 thread_pool / io_scheduler /
/// run_loop 是同一套语义（spawn_detached / spawn_joinable / resume / shutdown /
/// size / empty）。m_size 计数与自删除任务的所有权语义照搬 thread_pool，仅把
/// "队列 + 工作线程" 替换为 "当前线程内联 resume()"。
class inline_scheduler final : public IScheduler {
    struct Impl;
    std::unique_ptr<Impl> m_impl;

  public:
    inline_scheduler();
    ~inline_scheduler() override;

    inline_scheduler(const inline_scheduler &) = delete;
    inline_scheduler(inline_scheduler &&) = delete;
    auto operator=(const inline_scheduler &) -> inline_scheduler & = delete;
    auto operator=(inline_scheduler &&) -> inline_scheduler & = delete;

    // —— IScheduler ——
    auto spawn_detached(task::task<void> &&task) noexcept -> bool override;
    auto spawn_joinable(task::task<void> &&task) noexcept -> task::task<void> override;
    auto resume(std::coroutine_handle<> handle) noexcept -> bool override;
    auto shutdown() noexcept -> void override;
    auto is_shutdown() const -> bool override;
    auto size() const noexcept -> std::size_t override;
    auto empty() const noexcept -> bool override { return size() == 0; }
};

} // namespace silicon::scheduler

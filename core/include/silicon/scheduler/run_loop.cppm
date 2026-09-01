module;

#include <coroutine>
#include <cstddef>
#include <memory>

export module silicon.scheduler:run_loop;

import silicon.scheduler.task;
import :facade;

export namespace silicon::scheduler {

/// @brief 单线程同步执行上下文（参考 stdexec::run_loop）。
///
/// run_loop 是一种在 *调用 run() 的线程上就地* 执行任务的调度器：所有通过
/// resume() / spawn_*() 提交的工作进入一个线程安全队列，run() 在该线程上逐一
/// 弹出并 resume，直到 finish()（等价于 shutdown()）被调用后，在排空当前队列后返回。
///
/// 与 thread_pool（多工作线程、工作窃取）和 io_scheduler（事件驱动）不同，
/// run_loop **不拥有线程**，因而适合：
///   - 单元测试：在测试线程上确定性地驱动一组协程；
///   - 把执行上下文嵌入某个特定线程（如主线程、UI 线程）；
///   - 作为 stdexec 风格的 "inline" 调度器使用。
///
/// 它满足 scheduler_facade，因此对 task_group 等上层组件而言与 thread_pool / io_scheduler
/// 是同一套语义（spawn_detached / spawn_joinable / resume / shutdown / size / empty）。
///
/// 线程安全与生命周期：
///   - resume() / spawn_*() / finish() / shutdown() 均可从任意线程并发调用；
///   - finish() / shutdown() 通常应在 *不同于* run() 所在线程的线程上调用，否则若
///     run() 正阻塞在空队列等待，同线程调用将无法被该线程观察到；
///   - 承载 run() 的线程必须比 run_loop 对象更晚销毁——析构函数仅调用 finish()
///     唤醒等待中的 run()，不会等待其退出（run_loop 不持有线程，无法 join）。
class run_loop final {
    struct impl;
    std::unique_ptr<impl> m_impl;

  public:
    run_loop();
    ~run_loop();

    run_loop(const run_loop &) = delete;
    run_loop(run_loop &&) = delete;
    run_loop & operator=(const run_loop &) = delete;
    run_loop & operator=(run_loop &&) = delete;

    /// @brief 在当前线程上运行事件循环，直到 finish() 被调用。
    ///
    /// 每次从队列取出一个协程句柄并 resume()；队列为空且未请求停止时阻塞等待
    /// （通过条件变量）。finish() 被调用后，run() 会排空当前队列再返回。
    void run() noexcept ;

    /// @brief 请求停止循环。唤醒阻塞中的 run()，使其在排空当前队列后返回。
    ///
    /// 幂等：多次调用只有第一次生效。通常从另一个线程调用（见类注释）。
    void finish() noexcept ;

    // —— scheduler_facade ——
    bool spawn_detached(task<void> &&task) noexcept ;
    task<void> spawn_joinable(task<void> &&t) noexcept ;
    bool resume(std::coroutine_handle<>) noexcept ;
    void shutdown() noexcept ;
    bool is_shutdown() const ;
    std::size_t size() const noexcept ;
    bool empty() const noexcept { return size() == 0; }
};

} // namespace silicon::scheduler

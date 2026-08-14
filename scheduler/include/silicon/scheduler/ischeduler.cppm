module;

#include <coroutine>
#include <cstddef>

export module silicon.scheduler:ischeduler;

import silicon.scheduler.task;

// proxy 门面：全局模块片段包含 dispatch 宏，随后 import silicon.proxy
// （宏不随 C++20 模块导出）。
#include <silicon/proxy/proxy_macros.h>

import silicon.proxy;

export namespace silicon::scheduler {

/// @brief 调度器统一行为规范（type-erased 门面，鸭子类型满足即可；取消抽象基类）。
///
/// silicon 中所有可执行协程的调度器（CPU 密集型的 thread_pool、事件驱动的
/// io_scheduler、inline_scheduler / parallel_scheduler / run_loop）只要满足本门面
/// 即可被 task_group / 上层组件以同一套语义驱动，无需继承。
///
/// 约定（对应原 i_scheduler 虚方法；参数按值声明，proxy 以右值转发到具体
/// `task<void> &&` 形参，noexcept 差异由具体类自行兼容）：
///   - spawn_detached / spawn_joinable（提交任务）
///   - resume（恢复裸协程句柄）
///   - shutdown / is_shutdown（生命周期）
///   - size / empty（负载观测）
///
/// 各实现独有的能力不进入本门面：
///   - thread_pool：thread_count / queue_size / queue_empty / schedule()
///   - io_scheduler：poll / schedule_after / yield_for / process_events
///
/// 模板便捷重载（schedule<return_type>、resume<range_type>）无法擦除，仍留在
/// 各具体类中，并以此处的约定为实现基础。
PRO_DEF_MEM_DISPATCH(MemSchedulerSpawnDetached, spawn_detached);
PRO_DEF_MEM_DISPATCH(MemSchedulerSpawnJoinable, spawn_joinable);
PRO_DEF_MEM_DISPATCH(MemSchedulerResume, resume);
PRO_DEF_MEM_DISPATCH(MemSchedulerShutdown, shutdown);
PRO_DEF_MEM_DISPATCH(MemSchedulerIsShutdown, is_shutdown);
PRO_DEF_MEM_DISPATCH(MemSchedulerSize, size);
PRO_DEF_MEM_DISPATCH(MemSchedulerEmpty, empty);

struct scheduler_facade : silicon::proxy::facade_builder
    ::add_convention<MemSchedulerSpawnDetached,
                     bool(silicon::scheduler::task<void>)>
    ::add_convention<MemSchedulerSpawnJoinable,
                     silicon::scheduler::task<void>(silicon::scheduler::task<void>)>
    ::add_convention<MemSchedulerResume, bool(std::coroutine_handle<>)>
    ::add_convention<MemSchedulerShutdown, void()>
    ::add_convention<MemSchedulerIsShutdown, bool() const>
    ::add_convention<MemSchedulerSize, std::size_t() const>
    ::add_convention<MemSchedulerEmpty, bool() const>::build {};

using scheduler_proxy = silicon::proxy::proxy<scheduler_facade>;
using scheduler_view = silicon::proxy::proxy_view<scheduler_facade>;

template <class T, class... Args>
[[nodiscard]] scheduler_proxy make_scheduler(Args &&...args) {
    return silicon::proxy::make_proxy<scheduler_facade, T>(
        std::forward<Args>(args)...);
}

} // namespace silicon::scheduler

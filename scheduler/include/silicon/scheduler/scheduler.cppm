module;

export module silicon.scheduler;

// scheduler 位于 coroutine 之上：事件循环基础设施（io_notifier / poll_info /
// timer_handle）已归属本模块，pipe 与协程类型仍复用 coroutine。依赖单向：
// silicon.scheduler -> silicon.coroutine -> silicon.scheduler.task，无环。
//
// 两者都在本模块的公共 API 签名中出现（如 io_scheduler::poll 返回
// silicon::coroutine::task<poll_status>），故一并 re-export，消费方只需
// import silicon.scheduler 即可拿到完整类型。
export import silicon.coroutine;
export import silicon.scheduler.task;

export import :config;
export import :detail.poll_info;
export import :io_notifier;
export import :detail.timer_handle;
export import :ischeduler;
export import :thread_pool;
export import :io_scheduler;
export import :default_executor;

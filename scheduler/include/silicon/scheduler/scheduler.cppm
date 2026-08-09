module;

export module silicon.scheduler;

// scheduler 位于 coroutine 之下：事件循环基础设施（io_notifier / poll_info /
// timer_handle）与其依赖的调度原语（poll / fd / time / expected / sync_wait /
// concepts / awaiter_list / pipe）均已归属本模块。依赖单向：
// silicon.coroutine -> silicon.scheduler -> silicon.scheduler.task，无环。
//
// 注意：迁入的原语保留原有的 silicon::coroutine 命名空间，仅模块归属发生变化，
// 因此既有消费方（network 等）的类型书写不受影响。
export import silicon.scheduler.task;

export import :config;

// —— 自 silicon.coroutine 迁入的调度原语（命名空间仍为 silicon::coroutine）——
export import :concepts.awaitable;
export import :concepts.buffer;
export import :concepts.executor;
export import :concepts.promise;
export import :concepts.range_of;
export import :detail.awaiter_list;
export import :detail.pipe;
export import :expected;
export import :fd;
export import :poll;
export import :sync_wait;
export import :time;

// —— 事件循环基础设施 ——
export import :detail.poll_info;
export import :io_notifier;
export import :detail.timer_handle;
export import :ischeduler;
export import :thread_pool;
export import :io_scheduler;
export import :run_loop;
export import :default_executor;

module;

#include <coroutine>
#include <optional>

export module silicon.scheduler;
export import silicon.scheduler.error;

// std 可见性：本模块无 :config 分区（版本信息统一由
// silicon.core::get_version* 提供），在主接口的模块导入区 import std
//（注意：必须位于 export module 之后——全局模块片段内
// 出现 import 声明属非法，MSVC 报 C5202 且本项目警告视为错误），实现
// 单元（io_notifier_* / timer_handle 后端等 .cpp）不必逐一显式 include
// <mutex>/<vector> 等头。
import std;

// scheduler 位于 coroutine 之下：事件循环基础设施（io_notifier / poll_info /
// timer_handle）与其依赖的调度原语（poll / fd / time / expected / sync_wait /
// concepts / awaiter_list / pipe）均已归属本模块。依赖单向：
// silicon.coroutine -> silicon.scheduler -> silicon.scheduler.task，无环。
//
// 注意：迁入的原语保留原有的 silicon::coroutine 命名空间，仅模块归属发生变化，
// 因此既有消费方（network 等）的类型书写不受影响。
export import silicon.scheduler.task;

// —— 自 silicon.coroutine 迁入的调度原语（命名空间仍为 silicon::coroutine）——
export import :concepts.awaitable;
export import :concepts.buffer;
export import :concepts.executor;
export import :concepts.promise;
export import :concepts.range_of;
export import :awaiter_list;
export import :pipe;
export import :expected;
export import :fd;
export import :poll;
export import :sync_wait;
export import :time;

// —— 事件循环基础设施 ——
export import :poll_info;
export import :io_notifier;
export import :timer_handle;
export import :facade;
export import :thread_pool;
export import :io_scheduler;
export import :run_loop;
export import :inline_scheduler;
export import :parallel_scheduler;
export import :default_executor;

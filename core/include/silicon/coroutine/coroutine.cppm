module;

export module silicon.coroutine;
export import silicon.coroutine.error;

// std 可见性：原由 :config 分区 GMF 的 import std 承担（MSVC 特性——主接口
// import 对本模块实现单元可见）。:config 已收敛（版本信息由 silicon.core 提供），
// 故移入主接口导入区 import std（必须位于 export module 之后——全局模块片段内
// import 非法，MSVC C5202 且本项目警告视为错误）。
import std;

import silicon.scheduler.task;

// ---------------------------------------------------------------------------
// 依赖方向：silicon.coroutine -> silicon.scheduler -> silicon.scheduler.task。
//
// 调度相关的原语（concepts / expected / fd / poll / time / sync_wait /
// detail.awaiter_list / detail.pipe）连同事件循环基础设施（io_notifier /
// poll_info / timer_handle / io_scheduler）已下沉到 silicon.scheduler。
// 它们的命名空间保持 silicon::coroutine 不变，故此处整体 re-export 后，
// 既有消费方仍可通过 `import silicon.coroutine;` 拿到完整的公共 API。
// ---------------------------------------------------------------------------
export import silicon.scheduler;

// ---------------------------------------------------------------------------
// Re-export every coroutine partition through the primary module interface so
// that `import silicon.coroutine;` exposes the full public API. Each partition
// declares its types directly in namespace silicon::coroutine (or a sub-namespace),
// so no `using`-re-export shim is needed here.
// ---------------------------------------------------------------------------
export import :channel;
export import :facade;
export import :event;
export import :generator;
export import :invoke;
export import :latch;
export import :mutex;
export import :queue;
export import :ring_buffer;
export import :semaphore;
export import :shared_mutex;
export import :task_container;
export import :void_value;
export import :when_all;
export import :when_any;
export import :coroutine_pool;

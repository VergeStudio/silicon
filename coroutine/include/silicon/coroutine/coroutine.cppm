module;

export module silicon.coroutine;

import silicon.scheduler.task;

// ---------------------------------------------------------------------------
// Re-export every coroutine partition through the primary module interface so
// that `import silicon.coroutine;` exposes the full public API. Each partition
// declares its types directly in namespace silicon::coroutine (or a sub-namespace),
// so no `using`-re-export shim is needed here.
// ---------------------------------------------------------------------------
export import :concepts.awaitable;
export import :concepts.buffer;
export import :concepts.executor;
export import :concepts.promise;
export import :concepts.range_of;
export import :channel;
export import :condition_variable;
export import :detail.awaiter_list;
// 平台专属 io_notifier 后端已合并进单一 :io_notifier 分区（src/io_notifier.cppm）：
// 该分区内部按平台宏展开对应后端类声明，三个同名 .cpp 实现单元由宏开关决定是否
// 编译。此处只需无条件 re-export 单一 :io_notifier 分区即可（见下方 export import :io_notifier）。
export import :detail.pipe;
export import :detail.poll_info;
export import :detail.task_self_deleting;
export import :detail.timer_handle;
export import :detail.void_value;
export import :event;
export import :expected;
export import :fd;
export import :generator;
export import :invoke;
export import :io_notifier;
export import :latch;
export import :mutex;
export import :poll;
export import :queue;
export import :ring_buffer;
export import :semaphore;
export import :shared_mutex;
export import :sync_wait;
export import :task;
export import :task_container;
export import :task_group;
export import :time;
export import :when_all;
export import :when_any;
export import :config;

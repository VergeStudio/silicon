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
// 注：io_notifier / detail.poll_info / detail.timer_handle 已迁出到 silicon.scheduler
// （其唯一消费方是 io_scheduler，且 poll_info 的实现细节需被同模块 TU 直接触及）。
export import :detail.pipe;
export import :detail.task_self_deleting;
export import :detail.void_value;
export import :event;
export import :expected;
export import :fd;
export import :generator;
export import :invoke;
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

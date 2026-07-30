module;

export module silicon.coroutine;

import silicon.task;

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
export import :condition_variable;
export import :default_executor;
export import :detail.awaiter_list;
// 平台专属 io_notifier 后端：仅 import 当前平台对应分区（其余源文件已在
// xmake.lua 中按平台 remove_files，BMI 不存在）。
#if defined(_WIN32)
export import :detail.io_notifier_iocp;
#elif defined(__FreeBSD__) || defined(__APPLE__) || defined(__OpenBSD__) || defined(__NetBSD__)
export import :detail.io_notifier_kqueue;
#elif defined(__linux__)
export import :detail.io_notifier_epoll;
#endif
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
export import :pool;
export import :queue;
export import :ring_buffer;
export import :scheduler;
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

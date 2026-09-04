module;



#ifdef LIBCORO_FEATURE_NETWORKING
#endif // #ifdef LIBCORO_FEATURE_NETWORKING

#include <chrono>
#include <concepts>
#include <coroutine>
#include <utility>

export module silicon.scheduler:concepts.executor;

import :concepts.awaitable;
import :fd;
import silicon.scheduler.task;
import :poll;

export namespace silicon::scheduler::concepts {

// clang-format off
template<typename executor_type>
concept executor = requires(executor_type e, std::coroutine_handle<> c)
{
    { e.schedule() } -> silicon::scheduler::concepts::awaiter;
    { e.spawn_detached(std::declval<silicon::scheduler::task<void>>()) } -> std::same_as<bool>;
    { e.spawn_joinable(std::declval<silicon::scheduler::task<void>>()) } -> std::same_as<silicon::scheduler::task<void>>;
    { e.yield() } -> silicon::scheduler::concepts::awaiter;
    { e.resume(c) } -> std::same_as<bool>;
    { e.size() } -> std::same_as<std::size_t>;
    { e.empty() } -> std::same_as<bool>;
    { e.shutdown() } -> std::same_as<void>;
};

// io_executor 仅依赖 coroutine 自身类型（fd_t/poll_op/task），与 network 无耦合。
// 上游 libcoro 将其置于 LIBCORO_FEATURE_NETWORKING 之下属过度约束；silicon::network
// 的公共头（dns/resolver 等）无条件引用此概念，故此处无条件导出。
template<typename executor_type>
concept io_executor = executor<executor_type> and requires(executor_type e, std::coroutine_handle<> c, fd_t fd, silicon::scheduler::poll_op op, std::chrono::milliseconds timeout)
{
    { e.poll(fd, op, timeout) } -> std::same_as<silicon::scheduler::task<poll_status>>;
};

// clang-format on

} // namespace silicon::scheduler::concepts

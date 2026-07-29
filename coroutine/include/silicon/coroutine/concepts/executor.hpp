#pragma once

#include "silicon/coroutine/concepts/awaitable.hpp"
#include "silicon/coroutine/fd.hpp"
#include "silicon/coroutine/task.hpp"

#ifdef LIBCORO_FEATURE_NETWORKING
#    include "silicon/coroutine/poll.hpp"
#endif // #ifdef LIBCORO_FEATURE_NETWORKING

#include <chrono>
#include <concepts>
#include <coroutine>
#include <utility>

namespace silicon::coroutine::concepts {

// clang-format off
template<typename executor_type>
concept executor = requires(executor_type e, std::coroutine_handle<> c)
{
    { e.schedule() } -> silicon::coroutine::concepts::awaiter;
    { e.spawn_detached(std::declval<silicon::coroutine::task<void>>()) } -> std::same_as<bool>;
    { e.spawn_joinable(std::declval<silicon::coroutine::task<void>>()) } -> std::same_as<silicon::coroutine::task<void>>;
    { e.yield() } -> silicon::coroutine::concepts::awaiter;
    { e.resume(c) } -> std::same_as<bool>;
    { e.size() } -> std::same_as<std::size_t>;
    { e.empty() } -> std::same_as<bool>;
    { e.shutdown() } -> std::same_as<void>;
};

#ifdef LIBCORO_FEATURE_NETWORKING
template<typename executor_type>
concept io_executor = executor<executor_type> and requires(executor_type e, std::coroutine_handle<> c, fd_t fd, silicon::coroutine::poll_op op, std::chrono::milliseconds timeout)
{
    { e.poll(fd, op, timeout) } -> std::same_as<silicon::coroutine::task<poll_status>>;
};
#endif // #ifdef LIBCORO_FEATURE_NETWORKING

// clang-format on

} // namespace silicon::coroutine::concepts

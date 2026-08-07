module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <variant>


// EMSCRIPTEN does not currently support std::jthread or std::stop_source|token.
#ifndef EMSCRIPTEN
#    include <atomic>
#    include <cassert>
#    include <coroutine>
#    include <optional>
#    include <stop_token>
#    include <utility>
#    include <vector>
#endif

// 模块声明必须无条件出现，否则该文件在 EMSCRIPTEN 下不构成模块单元，
// 扫描器将报 missing :when_any dependency。分区内容整体受条件保护。
export module silicon.coroutine:when_any;

#ifndef EMSCRIPTEN

import silicon.scheduler;
import :detail.task_self_deleting;
import :event;
import :mutex;
import :task;
import :when_all;

export namespace silicon::coroutine {

namespace detail {

template<typename T>
struct when_any_variant_traits {
    using type = T;
};

template<>
struct when_any_variant_traits<void> {
    using type = std::monostate;
};

template<size_t index, typename return_type, concepts::awaitable awaitable>
auto make_when_any_tuple_task(
        std::atomic<bool> &first_completed, silicon::coroutine::event &notify, std::optional<return_type> &return_value, awaitable a
)
        -> silicon::coroutine::task<void> {
    auto expected = false;
    if constexpr(concepts::awaitable_void<awaitable>) {
        co_await static_cast<awaitable &&>(a);
        if(first_completed.compare_exchange_strong(
                   expected, true, std::memory_order::acq_rel, std::memory_order::relaxed
           )) {
            return_value.emplace(std::in_place_index<index>, std::monostate{});
            notify.set();
        }
    } else {
        auto result = co_await static_cast<awaitable &&>(a);
        if(first_completed.compare_exchange_strong(
                   expected, true, std::memory_order::acq_rel, std::memory_order::relaxed
           )) {
            return_value.emplace(std::in_place_index<index>, std::move(result));
            notify.set();
        }
    }
    co_return;
}

template<typename return_type, concepts::awaitable... awaitable_type, size_t... indices>
[[nodiscard]] auto make_when_any_tuple_controller_task(
        std::index_sequence<indices...>,
        silicon::coroutine::event &notify,
        std::optional<return_type> &return_value,
        awaitable_type... awaitables
) -> silicon::coroutine::detail::task_self_deleting {
    std::atomic<bool> first_completed{false};
    co_await silicon::coroutine::when_all(
            make_when_any_tuple_task<indices>(first_completed, notify, return_value, std::move(awaitables))...
    );
    co_return;
}

template<concepts::awaitable awaitable>
auto make_when_any_task_return_void(awaitable a, std::atomic<bool> &first_completed, silicon::coroutine::event &notify)
        -> silicon::coroutine::task<void> {
    co_await static_cast<awaitable &&>(a);
    auto expected = false;
    if(first_completed.compare_exchange_strong(expected, true, std::memory_order::acq_rel, std::memory_order::relaxed)) {
        notify.set(); // This will trigger the controller task to wake up exactly once.
    }
    co_return;
}

template<concepts::awaitable awaitable, typename return_type>
auto make_when_any_task(
        awaitable a, std::atomic<bool> &first_completed, silicon::coroutine::event &notify, std::optional<return_type> &return_value
)
        -> silicon::coroutine::task<void> {
    auto expected = false;
    auto result = co_await static_cast<awaitable &&>(a);
    // Its important to only touch return_value and notify once since their lifetimes will be destroyed
    // after being set and notified the first time.
    if(first_completed.compare_exchange_strong(expected, true, std::memory_order::acq_rel, std::memory_order::relaxed)) {
        return_value = std::move(result);
        notify.set();
    }

    co_return;
}

template<std::ranges::range range_type, concepts::awaitable awaitable_type = std::ranges::range_value_t<range_type>>
auto make_when_any_controller_task_return_void(range_type awaitables, silicon::coroutine::event &notify)
        -> silicon::coroutine::detail::task_self_deleting {
    std::atomic<bool> first_completed{false};
    std::vector<silicon::coroutine::task<void>> tasks{};

    if constexpr(std::ranges::sized_range<range_type>) {
        tasks.reserve(std::size(awaitables));
    }

    for(auto &&a: awaitables) {
        tasks.emplace_back(make_when_any_task_return_void<awaitable_type>(std::move(a), first_completed, notify));
    }

    co_await silicon::coroutine::when_all(std::move(tasks));
    co_return;
}

template<
        std::ranges::range range_type,
        concepts::awaitable awaitable_type = std::ranges::range_value_t<range_type>,
        typename return_type = typename concepts::awaitable_traits<awaitable_type>::awaiter_return_type,
        typename return_type_base = std::remove_reference_t<return_type>>
auto make_when_any_controller_task(
        range_type awaitables, silicon::coroutine::event &notify, std::optional<return_type_base> &return_value
)
        -> silicon::coroutine::detail::task_self_deleting {
    // This must live for as long as the longest running when_any task since each task tries to see
    // if it was the first to complete. Only the very first task to complete will set the return_value
    // and notify.
    std::atomic<bool> first_completed{false};

    // This detatched task will maintain the lifetime of all the when_any tasks.
    std::vector<silicon::coroutine::task<void>> tasks{};

    if constexpr(std::ranges::sized_range<range_type>) {
        tasks.reserve(std::size(awaitables));
    }

    for(auto &&a: awaitables) {
        tasks.emplace_back(
                make_when_any_task<awaitable_type, return_type_base>(std::move(a), first_completed, notify, return_value)
        );
    }

    co_await silicon::coroutine::when_all(std::move(tasks));
    co_return;
}

} // namespace detail

template<concepts::awaitable... awaitable_type>
[[nodiscard]] auto when_any(std::stop_source stop_source, awaitable_type... awaitables)
        -> silicon::coroutine::task<std::variant<typename detail::when_any_variant_traits<
                std::remove_reference_t<typename concepts::awaitable_traits<awaitable_type>::awaiter_return_type>>::type...>> {
    using return_type = std::variant<typename detail::when_any_variant_traits<
            std::remove_reference_t<typename concepts::awaitable_traits<awaitable_type>::awaiter_return_type>>::type...>;

    silicon::coroutine::event notify{};
    std::optional<return_type> return_value{std::nullopt};

    auto controller_task = detail::make_when_any_tuple_controller_task(
            std::index_sequence_for<awaitable_type...>{},
            notify,
            return_value,
            std::forward<awaitable_type>(awaitables)...
    );
    controller_task.handle().resume();

    co_await notify;
    stop_source.request_stop();
    co_return std::move(return_value.value());
}

template<concepts::awaitable... awaitable_type>
[[nodiscard]] auto when_any(awaitable_type... awaitables)
        -> silicon::coroutine::task<std::variant<typename detail::when_any_variant_traits<
                std::remove_reference_t<typename concepts::awaitable_traits<awaitable_type>::awaiter_return_type>>::type...>> {
    using return_type = std::variant<typename detail::when_any_variant_traits<
            std::remove_reference_t<typename concepts::awaitable_traits<awaitable_type>::awaiter_return_type>>::type...>;

    silicon::coroutine::event notify{};
    std::optional<return_type> return_value{std::nullopt};

    auto controller_task = detail::make_when_any_tuple_controller_task(
            std::index_sequence_for<awaitable_type...>{},
            notify,
            return_value,
            std::forward<awaitable_type>(awaitables)...
    );
    controller_task.handle().resume();

    co_await notify;
    co_return std::move(return_value.value());
}

template<
        std::ranges::range range_type,
        concepts::awaitable awaitable_type = std::ranges::range_value_t<range_type>,
        typename return_type = typename concepts::awaitable_traits<awaitable_type>::awaiter_return_type,
        typename return_type_base = std::remove_reference_t<return_type>>
[[nodiscard]] auto when_any(std::stop_source stop_source, range_type awaitables) -> silicon::coroutine::task<return_type_base> {
    silicon::coroutine::event notify{};

    if constexpr(std::is_void_v<return_type_base>) {
        auto controller_task =
                detail::make_when_any_controller_task_return_void(std::forward<range_type>(awaitables), notify);
        controller_task.handle().resume();

        co_await notify;
        stop_source.request_stop();
        co_return;
    } else {
        // Using an std::optional to prevent the need to default construct the type on the stack.
        std::optional<return_type_base> return_value{std::nullopt};

        auto controller_task =
                detail::make_when_any_controller_task(std::forward<range_type>(awaitables), notify, return_value);
        controller_task.handle().resume();

        co_await notify;
        stop_source.request_stop();
        co_return std::move(return_value.value());
    }
}

template<
        std::ranges::range range_type,
        concepts::awaitable awaitable_type = std::ranges::range_value_t<range_type>,
        typename return_type = typename concepts::awaitable_traits<awaitable_type>::awaiter_return_type,
        typename return_type_base = std::remove_reference_t<return_type>>
[[nodiscard]] auto when_any(range_type awaitables) -> silicon::coroutine::task<return_type_base> {
    silicon::coroutine::event notify{};

    if constexpr(std::is_void_v<return_type_base>) {
        auto controller_task =
                detail::make_when_any_controller_task_return_void(std::forward<range_type>(awaitables), notify);
        controller_task.handle().resume();

        co_await notify;
        co_return;
    } else {
        std::optional<return_type_base> return_value{std::nullopt};

        auto controller_task =
                detail::make_when_any_controller_task(std::forward<range_type>(awaitables), notify, return_value);
        controller_task.handle().resume();

        co_await notify;
        co_return std::move(return_value.value());
    }
}

} // namespace silicon::coroutine

#endif // EMSCRIPTEN

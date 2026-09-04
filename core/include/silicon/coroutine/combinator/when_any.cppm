module;


#include <variant>



#ifndef EMSCRIPTEN
#    include <atomic>
#    include <cassert>
#    include <coroutine>
#    include <optional>
#    include <stop_token>
#    include <utility>
#    include <vector>
#endif



export module silicon.coroutine:when_any;

#ifndef EMSCRIPTEN

import silicon.scheduler;
import silicon.scheduler.task;
import :event;
import :mutex;
import :when_all;

export namespace silicon::coroutine {



template<typename T>
struct when_any_variant_traits {
    using type = T;
};

template<>
struct when_any_variant_traits<void> {
    using type = std::monostate;
};

template<size_t index, typename return_type, silicon::scheduler::concepts::awaitable awaitable>
silicon::scheduler::task<void> make_when_any_tuple_task(
        std::atomic<bool> &first_completed, silicon::coroutine::event &notify, std::optional<return_type> &return_value, awaitable a
) {
    auto expected = false;
    if constexpr(silicon::scheduler::concepts::awaitable_void<awaitable>) {
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

template<typename return_type, silicon::scheduler::concepts::awaitable... awaitable_type, size_t... indices>
[[nodiscard]] silicon::scheduler::task_self_deleting make_when_any_tuple_controller_task(
        std::index_sequence<indices...>,
        silicon::coroutine::event &notify,
        std::optional<return_type> &return_value,
        awaitable_type... awaitables
) {
    std::atomic<bool> first_completed{false};
    co_await silicon::coroutine::when_all(
            make_when_any_tuple_task<indices>(first_completed, notify, return_value, std::move(awaitables))...
    );
    co_return;
}

template<silicon::scheduler::concepts::awaitable awaitable>
silicon::scheduler::task<void> make_when_any_task_return_void(awaitable a, std::atomic<bool> &first_completed, silicon::coroutine::event &notify) {
    co_await static_cast<awaitable &&>(a);
    auto expected = false;
    if(first_completed.compare_exchange_strong(expected, true, std::memory_order::acq_rel, std::memory_order::relaxed)) {
        notify.set();
    }
    co_return;
}

template<silicon::scheduler::concepts::awaitable awaitable, typename return_type>
silicon::scheduler::task<void> make_when_any_task(
        awaitable a, std::atomic<bool> &first_completed, silicon::coroutine::event &notify, std::optional<return_type> &return_value
) {
    auto expected = false;
    auto result = co_await static_cast<awaitable &&>(a);


    if(first_completed.compare_exchange_strong(expected, true, std::memory_order::acq_rel, std::memory_order::relaxed)) {
        return_value = std::move(result);
        notify.set();
    }

    co_return;
}

template<std::ranges::range range_type, silicon::scheduler::concepts::awaitable awaitable_type = std::ranges::range_value_t<range_type>>
silicon::scheduler::task_self_deleting make_when_any_controller_task_return_void(range_type awaitables, silicon::coroutine::event &notify) {
    std::atomic<bool> first_completed{false};
    std::vector<silicon::scheduler::task<void>> tasks{};

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
        silicon::scheduler::concepts::awaitable awaitable_type = std::ranges::range_value_t<range_type>,
        typename return_type = typename silicon::scheduler::concepts::awaitable_traits<awaitable_type>::awaiter_return_type,
        typename return_type_base = std::remove_reference_t<return_type>>
silicon::scheduler::task_self_deleting make_when_any_controller_task(
        range_type awaitables, silicon::coroutine::event &notify, std::optional<return_type_base> &return_value
) {



    std::atomic<bool> first_completed{false};


    std::vector<silicon::scheduler::task<void>> tasks{};

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



template<silicon::scheduler::concepts::awaitable... awaitable_type>
[[nodiscard]] silicon::scheduler::task<std::variant<typename when_any_variant_traits<
                std::remove_reference_t<typename silicon::scheduler::concepts::awaitable_traits<awaitable_type>::awaiter_return_type>>::type...>> when_any(std::stop_source stop_source, awaitable_type... awaitables) {
    using return_type = std::variant<typename when_any_variant_traits<
            std::remove_reference_t<typename silicon::scheduler::concepts::awaitable_traits<awaitable_type>::awaiter_return_type>>::type...>;

    silicon::coroutine::event notify{};
    std::optional<return_type> return_value{std::nullopt};

    auto controller_task = make_when_any_tuple_controller_task(
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

template<silicon::scheduler::concepts::awaitable... awaitable_type>
[[nodiscard]] silicon::scheduler::task<std::variant<typename when_any_variant_traits<
                std::remove_reference_t<typename silicon::scheduler::concepts::awaitable_traits<awaitable_type>::awaiter_return_type>>::type...>> when_any(awaitable_type... awaitables) {
    using return_type = std::variant<typename when_any_variant_traits<
            std::remove_reference_t<typename silicon::scheduler::concepts::awaitable_traits<awaitable_type>::awaiter_return_type>>::type...>;

    silicon::coroutine::event notify{};
    std::optional<return_type> return_value{std::nullopt};

    auto controller_task = make_when_any_tuple_controller_task(
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
        silicon::scheduler::concepts::awaitable awaitable_type = std::ranges::range_value_t<range_type>,
        typename return_type = typename silicon::scheduler::concepts::awaitable_traits<awaitable_type>::awaiter_return_type,
        typename return_type_base = std::remove_reference_t<return_type>>
[[nodiscard]] silicon::scheduler::task<return_type_base> when_any(std::stop_source stop_source, range_type awaitables) {
    silicon::coroutine::event notify{};

    if constexpr(std::is_void_v<return_type_base>) {
        auto controller_task =
                make_when_any_controller_task_return_void(std::forward<range_type>(awaitables), notify);
        controller_task.handle().resume();

        co_await notify;
        stop_source.request_stop();
        co_return;
    } else {

        std::optional<return_type_base> return_value{std::nullopt};

        auto controller_task =
                make_when_any_controller_task(std::forward<range_type>(awaitables), notify, return_value);
        controller_task.handle().resume();

        co_await notify;
        stop_source.request_stop();
        co_return std::move(return_value.value());
    }
}

template<
        std::ranges::range range_type,
        silicon::scheduler::concepts::awaitable awaitable_type = std::ranges::range_value_t<range_type>,
        typename return_type = typename silicon::scheduler::concepts::awaitable_traits<awaitable_type>::awaiter_return_type,
        typename return_type_base = std::remove_reference_t<return_type>>
[[nodiscard]] silicon::scheduler::task<return_type_base> when_any(range_type awaitables) {
    silicon::coroutine::event notify{};

    if constexpr(std::is_void_v<return_type_base>) {
        auto controller_task =
                make_when_any_controller_task_return_void(std::forward<range_type>(awaitables), notify);
        controller_task.handle().resume();

        co_await notify;
        co_return;
    } else {
        std::optional<return_type_base> return_value{std::nullopt};

        auto controller_task =
                make_when_any_controller_task(std::forward<range_type>(awaitables), notify, return_value);
        controller_task.handle().resume();

        co_await notify;
        co_return std::move(return_value.value());
    }
}

}

#endif

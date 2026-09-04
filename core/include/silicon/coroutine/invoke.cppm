module;

#include <utility>

export module silicon.coroutine:invoke;

export namespace silicon::coroutine {

template<typename functor_type, typename... args_types>
auto make_invoker_task(functor_type functor, args_types &&...args) -> decltype(functor(std::forward<args_types>(args)...)) {
    auto user_task = functor(std::forward<args_types>(args)...);
    co_return co_await user_task;
}

template<typename functor_type, typename... args_types>
decltype(auto) invoke(functor_type functor, args_types &&...args) {
    auto invoker_task = make_invoker_task(std::forward<functor_type>(functor), std::forward<args_types>(args)...);
    invoker_task.resume();
    return invoker_task;

}

}

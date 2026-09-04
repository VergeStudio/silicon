module;

#include <coroutine>
#include <cstddef>
#include <silicon/proxy/proxy_macros.h>
#include <tuple>

export module silicon.scheduler:facade;

import silicon.scheduler.task;

import silicon.proxy;

export namespace silicon::scheduler {

PRO_DEF_MEM_DISPATCH(MemSchedulerSpawnDetached, spawn_detached);
PRO_DEF_MEM_DISPATCH(MemSchedulerSpawnJoinable, spawn_joinable);
PRO_DEF_MEM_DISPATCH(MemSchedulerResume, resume);
PRO_DEF_MEM_DISPATCH(MemSchedulerShutdown, shutdown);
PRO_DEF_MEM_DISPATCH(MemSchedulerIsShutdown, is_shutdown);
PRO_DEF_MEM_DISPATCH(MemSchedulerSize, size);
PRO_DEF_MEM_DISPATCH(MemSchedulerEmpty, empty);

struct scheduler_facade : silicon::proxy::facade_builder
    ::add_convention<MemSchedulerSpawnDetached,
                     bool(silicon::scheduler::task<void>)>
    ::add_convention<MemSchedulerSpawnJoinable,
                     silicon::scheduler::task<void>(silicon::scheduler::task<void>)>
    ::add_convention<MemSchedulerResume, bool(std::coroutine_handle<>)>
    ::add_convention<MemSchedulerShutdown, void()>
    ::add_convention<MemSchedulerIsShutdown, bool() const>
    ::add_convention<MemSchedulerSize, std::size_t() const>
    ::add_convention<MemSchedulerEmpty, bool() const>::build {};

using scheduler_proxy = silicon::proxy::proxy<scheduler_facade>;
using scheduler_view = silicon::proxy::proxy_view<scheduler_facade>;

template <class T, class... Args>
[[nodiscard]] scheduler_proxy make_scheduler(Args &&...args) {
    return silicon::proxy::make_proxy<scheduler_facade, T>(
        std::forward<Args>(args)...);
}

}

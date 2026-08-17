module;

// 标准库头必须置于全局模块片段：接口单元全局片段中的 #include 对实现单元不可达。
#include <atomic>
#include <memory>

module silicon.coroutine;

namespace silicon::coroutine {

/// Implementation state of silicon::coroutine::condition_variable.
struct condition_variable::impl {
  public:
    /// @brief The list of waiters.
    std::atomic<awaiter_base *> m_awaiters{nullptr};
};

condition_variable::condition_variable(): m_p(std::make_unique<impl>()) {}

condition_variable::~condition_variable() = default;

auto condition_variable::pop_all_waiters() noexcept -> awaiter_base * {
    return awaiter_list_pop_all(m_p->m_awaiters);
}

auto condition_variable::push_waiter(awaiter_base *waiter) noexcept -> void {
    awaiter_list_push(m_p->m_awaiters, waiter);
}

auto condition_variable::make_notify_all_executor_individual_task(awaiter_base *waiter)
        -> silicon::scheduler::task<void> {
    switch(co_await waiter->on_notify()) {
        case notify_status_t::kNotReady:
            // Re-enqueue since the predicate isn't ready and return since the notify has been satisfied.
            awaiter_list_push(m_p->m_awaiters, waiter);
            break;
        case notify_status_t::kReady:
        case notify_status_t::kAwaiterDead:
            // Don't re-enqueue any awaiters that are ready or dead.
            break;
    }
}

condition_variable::awaiter_base::awaiter_base(
        silicon::coroutine::condition_variable &cv,
        silicon::coroutine::scoped_lock &l
)
    : m_condition_variable(cv),
      m_lock(l) {
}

condition_variable::awaiter::awaiter(
        silicon::coroutine::condition_variable &cv,
        silicon::coroutine::scoped_lock &l
) noexcept
    : awaiter_base(cv, l) {
    strategy_ = make_notify<notify_strategy<awaiter>>(this);
}

auto condition_variable::awaiter::await_ready() const noexcept -> bool {
    return false;
}

auto condition_variable::awaiter::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
    m_awaiting_coroutine = awaiting_coroutine;
    silicon::coroutine::awaiter_list_push(m_condition_variable.m_p->m_awaiters, static_cast<awaiter_base *>(this));
    m_lock.owned_mutex()->unlock();
    return true;
}

auto condition_variable::awaiter::do_on_notify() -> silicon::scheduler::task<condition_variable::notify_status_t> {
    // Re-lock, the waiter is now responsible for unlocking.
    co_await m_lock.owned_mutex()->lock();
    m_awaiting_coroutine.resume();
    co_return notify_status_t::kReady;
}

condition_variable::awaiter_with_predicate::awaiter_with_predicate(
        silicon::coroutine::condition_variable &cv,
        silicon::coroutine::scoped_lock &l,
        condition_variable::predicate_type p
) noexcept
    : awaiter_base(cv, l),
      m_predicate(std::move(p)) {
    strategy_ = make_notify<notify_strategy<awaiter_with_predicate>>(this);
}

auto condition_variable::awaiter_with_predicate::await_ready() const noexcept -> bool {
    return m_predicate();
}

auto condition_variable::awaiter_with_predicate::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
    m_awaiting_coroutine = awaiting_coroutine;
    silicon::coroutine::awaiter_list_push(m_condition_variable.m_p->m_awaiters, static_cast<awaiter_base *>(this));
    m_lock.owned_mutex()->unlock();
    return true;
}

auto condition_variable::awaiter_with_predicate::do_on_notify() -> silicon::scheduler::task<condition_variable::notify_status_t> {
    co_await m_lock.owned_mutex()->lock();
    if(m_predicate()) {
        m_awaiting_coroutine.resume();
        co_return notify_status_t::kReady;
    }

    m_lock.owned_mutex()->unlock();
    co_return notify_status_t::kNotReady;
}

#ifndef EMSCRIPTEN

condition_variable::awaiter_with_predicate_stop_token::awaiter_with_predicate_stop_token(
        silicon::coroutine::condition_variable &cv,
        silicon::coroutine::scoped_lock &l,
        condition_variable::predicate_type p,
        std::stop_token stop_token
) noexcept
    : awaiter_base(cv, l),
      m_predicate(std::move(p)),
      m_stop_token(std::move(stop_token)) {
    strategy_ = make_notify<notify_strategy<awaiter_with_predicate_stop_token>>(this);
}

auto condition_variable::awaiter_with_predicate_stop_token::await_ready() noexcept -> bool {
    m_predicate_result = m_predicate();
    return m_predicate_result;
}

auto condition_variable::awaiter_with_predicate_stop_token::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
    m_awaiting_coroutine = awaiting_coroutine;
    silicon::coroutine::awaiter_list_push(m_condition_variable.m_p->m_awaiters, static_cast<awaiter_base *>(this));
    m_lock.owned_mutex()->unlock();
    return true;
}

auto condition_variable::awaiter_with_predicate_stop_token::do_on_notify() -> silicon::scheduler::task<condition_variable::notify_status_t> {
    co_await m_lock.owned_mutex()->lock();
    m_predicate_result = m_predicate();

    // If the predicate is ready or a stop has been requested resume.
    if(m_predicate_result || m_stop_token.stop_requested()) {
        m_awaiting_coroutine.resume();
        co_return notify_status_t::kReady;
    }

    m_lock.owned_mutex()->unlock();
    co_return notify_status_t::kNotReady;
}

#endif

#ifdef LIBCORO_FEATURE_NETWORKING

condition_variable::controller_data::controller_data(
        std::optional<std::cv_status> &status,
        bool &predicate_result,
        std::optional<condition_variable::predicate_type> predicate,
        std::optional<const std::stop_token> stop_token
) noexcept
    : m_status(status),
      m_predicate_result(predicate_result),
      m_predicate(std::move(predicate)),
      m_stop_token({std::move(stop_token)}) {
}

condition_variable::awaiter_with_wait_hook::awaiter_with_wait_hook(
        silicon::coroutine::condition_variable &cv,
        silicon::coroutine::scoped_lock &l,
        condition_variable::controller_data &data
) noexcept
    : awaiter_base(cv, l),
      m_data(data) {
    strategy_ = make_notify<notify_strategy<awaiter_with_wait_hook>>(this);
}

auto condition_variable::awaiter_with_wait_hook::do_on_notify() -> silicon::scheduler::task<condition_variable::notify_status_t> {
    auto event_lock = co_await m_data.m_event_mutex.scoped_lock();

    // See if this awaiter is a real notify or if it has timed out already.
    if(m_data.m_awaiter_completed.load(std::memory_order::acquire)) {
        // This awaiter timed out, report as dead after killing/resuming the on notify callback task.
        event_lock.unlock();
        m_data.m_notify_callback.set();
        co_return notify_status_t::kAwaiterDead;
    }

    auto *waiter_mutex = m_lock.owned_mutex();
    co_await waiter_mutex->lock();

    // If there is no predicate then this awaiter is always ready on notify.
    if(!m_data.m_predicate.has_value()) {
        m_data.m_awaiter_completed.exchange(true, std::memory_order::release);
        m_data.m_status = {std::cv_status::no_timeout};
        event_lock.unlock();
        m_data.m_notify_callback.set();
        co_return notify_status_t::kReady;
    }

    m_data.m_predicate_result = m_data.m_predicate.value()();

    // If the predicate is ready or we've been requested to stop then we are ready.
    if(m_data.m_predicate_result || (m_data.m_stop_token.has_value() && m_data.m_stop_token.value().stop_requested())) {
        m_data.m_awaiter_completed.exchange(true, std::memory_order::release);
        m_data.m_status = {std::cv_status::no_timeout};
        event_lock.unlock();
        m_data.m_notify_callback.set();
        co_return notify_status_t::kReady;
    }

    waiter_mutex->unlock();
    co_return notify_status_t::kNotReady;
}

#endif

auto condition_variable::notify_one() -> silicon::scheduler::task<void> {
    // The loop is here in case there are *dead* awaiter_hook_tasks that need to be skipped.
    while(true) {
        auto *waiter = awaiter_list_pop(m_p->m_awaiters);
        if(waiter == nullptr) {
            co_return; // There is nobody to currently notify.
        }

        switch(co_await waiter->on_notify()) {
            case notify_status_t::kReady:
                // The predicate was ready and the awaiter is resumed.
                co_return;
            case notify_status_t::kNotReady:
                // Re-enqueue since the predicate isn't ready and return since the notify has been satisfied.
                silicon::coroutine::awaiter_list_push(m_p->m_awaiters, waiter);
                co_return;
            case notify_status_t::kAwaiterDead:
                // This is an awaiter_with_wait_hook that timed out, try the next awaiter.
                break;
        }
    }
}

auto condition_variable::notify_all() -> silicon::scheduler::task<void> {
    auto *waiter = awaiter_list_pop_all(m_p->m_awaiters);

    while(waiter != nullptr) {
        // Need to grab next before notifying since the notifier will self destruct after completing.
        awaiter_base *next = waiter->m_next;

        switch(co_await waiter->on_notify()) {
            case notify_status_t::kNotReady:
                // Re-enqueue since the predicate isn't ready and return since the notify has been satisfied.
                silicon::coroutine::awaiter_list_push(m_p->m_awaiters, waiter);
                break;
            case notify_status_t::kReady:
            case notify_status_t::kAwaiterDead:
                // Don't re-enqueue any awaiters that are ready or dead.
                break;
        }

        waiter = next;
    }

    co_return;
}

auto condition_variable::wait(silicon::coroutine::scoped_lock &lock) -> awaiter {
    return awaiter{*this, lock};
}

auto condition_variable::wait(
        silicon::coroutine::scoped_lock &lock,
        condition_variable::predicate_type predicate
) -> awaiter_with_predicate {
    return awaiter_with_predicate{*this, lock, std::move(predicate)};
}

#ifndef EMSCRIPTEN

auto condition_variable::wait(
        silicon::coroutine::scoped_lock &lock,
        std::stop_token stop_token,
        condition_variable::predicate_type predicate
) -> awaiter_with_predicate_stop_token {
    return awaiter_with_predicate_stop_token{*this, lock, std::move(predicate), std::move(stop_token)};
}

#endif

} // namespace silicon::coroutine

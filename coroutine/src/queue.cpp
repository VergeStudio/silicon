module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
// 标准库头必须置于全局模块片段：接口单元全局片段中的 #include 对实现单元不可达。
#include <atomic>
#include <coroutine>
#include <memory>
#include <optional>
#include <queue>
#include <type_traits>
#include <utility>

module silicon.coroutine;

namespace silicon::coroutine {

// ===========================================================================
// queue::awaiter
// ===========================================================================

template<typename element_type>
queue<element_type>::awaiter::awaiter(queue<element_type> &q) noexcept
    : m_queue(q) {}

template<typename element_type>
bool queue<element_type>::awaiter::await_ready() noexcept {
    // This awaiter is ready when it has actually acquired an element or it is shutting down.
    if(m_queue.m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped) {
        m_queue.m_p->m_mutex.unlock();
        return true; // await_resume with stopped
    }

    // If we have items return it.
    if(!m_queue.empty()) {
        if constexpr(std::is_move_constructible_v<element_type>) {
            m_element = std::move(m_queue.m_p->m_elements.front());
        } else {
            m_element = m_queue.m_p->m_elements.front();
        }

        m_queue.m_p->m_elements.pop();
        m_queue.m_p->m_mutex.unlock();
        return true;
    }

    // Nothing available suspend, mutex will be unlocked in await_suspend.
    return false;
}

template<typename element_type>
bool queue<element_type>::awaiter::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
    // No element is ready, put ourselves on the waiter list and suspend.
    this->m_next = m_queue.m_p->m_waiters;
    m_queue.m_p->m_waiters = this;
    m_awaiting_coroutine = awaiting_coroutine;
    m_queue.m_p->m_mutex.unlock();
    return true;
}

template<typename element_type>
auto queue<element_type>::awaiter::await_resume() noexcept -> expected<element_type, queue_consume_result> {
    if(m_element.has_value()) {
        if constexpr(std::is_move_constructible_v<element_type>) {
            return std::move(m_element.value());
        } else {
            return m_element.value();
        }
    } else {
        // If we don't have an item the queue has stopped, the prior functions will have checked the state.
        return unexpected<queue_consume_result>(queue_consume_result::kStopped);
    }
}

// ===========================================================================
// queue
// ===========================================================================

template<typename element_type>
queue<element_type>::queue()
    : m_p(std::make_unique<impl>()) {}

template<typename element_type>
queue<element_type>::~queue() {
    // Synchronous, non-blocking shutdown to avoid deadlock when the
    // queue is destroyed from within a coroutine context. sync_wait()
    // would block the current thread, preventing the scheduler from
    // processing the shutdown coroutine.
    if(m_p->m_running_state.exchange(running_state_t::kStopped, std::memory_order::acq_rel) == running_state_t::kStopped) {
        return;
    }

    // Wake up any waiters without acquiring the coroutine mutex.
    auto *waiters = m_p->m_waiters;
    m_p->m_waiters = nullptr;
    while(waiters != nullptr) {
        auto *next = waiters->m_next;
        waiters->m_awaiting_coroutine.resume();
        waiters = next;
    }
}

template<typename element_type>
bool queue<element_type>::empty() const { return size() == 0; }

template<typename element_type>
std::size_t queue<element_type>::size() const {
    std::atomic_thread_fence(std::memory_order::acquire);
    return m_p->m_elements.size();
}

template<typename element_type>
silicon::scheduler::task<queue_produce_result> queue<element_type>::push(const element_type &element) {
    // The general idea is to see if anyone is waiting, and if so directly transfer the element
    // to that waiter. If there is nobody waiting then move the element into the queue.
    auto lock = co_await m_p->m_mutex.scoped_lock();

    if(m_p->m_running_state.load(std::memory_order::acquire) != running_state_t::kRunning) {
        co_return queue_produce_result::kStopped;
    }

    // assert(m_element.empty())
    if(m_p->m_waiters != nullptr) {
        auto *waiter = std::exchange(m_p->m_waiters, m_p->m_waiters->m_next);
        lock.unlock();

        // Transfer the element directly to the awaiter.
        waiter->m_element = element;
        waiter->m_awaiting_coroutine.resume();
    } else {
        m_p->m_elements.push(element);
    }

    co_return queue_produce_result::kProduced;
}

template<typename element_type>
silicon::scheduler::task<queue_produce_result> queue<element_type>::push(element_type &&element) {
    auto lock = co_await m_p->m_mutex.scoped_lock();

    if(m_p->m_running_state.load(std::memory_order::acquire) != running_state_t::kRunning) {
        co_return queue_produce_result::kStopped;
    }

    if(m_p->m_waiters != nullptr) {
        auto *waiter = std::exchange(m_p->m_waiters, m_p->m_waiters->m_next);
        lock.unlock();

        // Transfer the element directly to the awaiter.
        waiter->m_element = std::move(element);
        waiter->m_awaiting_coroutine.resume();
    } else {
        m_p->m_elements.push(std::move(element));
    }

    co_return queue_produce_result::kProduced;
}

template<typename element_type>
template<typename... args_type>
silicon::scheduler::task<queue_produce_result> queue<element_type>::emplace(args_type &&...args) {
    auto lock = co_await m_p->m_mutex.scoped_lock();

    if(m_p->m_running_state.load(std::memory_order::acquire) != running_state_t::kRunning) {
        co_return queue_produce_result::kStopped;
    }

    if(m_p->m_waiters != nullptr) {
        auto *waiter = std::exchange(m_p->m_waiters, m_p->m_waiters->m_next);
        lock.unlock();

        waiter->m_element.emplace(std::forward<args_type>(args)...);
        waiter->m_awaiting_coroutine.resume();
    } else {
        m_p->m_elements.emplace(std::forward<args_type>(args)...);
    }

    co_return queue_produce_result::kProduced;
}

template<typename element_type>
silicon::scheduler::task<expected<element_type, queue_consume_result>> queue<element_type>::pop() {
    co_await m_p->m_mutex.lock();
    co_return co_await awaiter{*this};
}

template<typename element_type>
auto queue<element_type>::try_pop() -> expected<element_type, queue_consume_result> {
    if(m_p->m_mutex.try_lock()) {
        // Capture mutex into a scoped lock to manage unlocking correctly.
        silicon::coroutine::scoped_lock lk{m_p->m_mutex};

        // Return if stopped.
        if(m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped) {
            return unexpected<queue_consume_result>(queue_consume_result::kStopped);
        }

        // Return if empty.
        if(empty()) {
            return unexpected<queue_consume_result>(queue_consume_result::kEmpty);
        }

        expected<element_type, queue_consume_result> value;
        if constexpr(std::is_move_constructible_v<element_type>) {
            value = std::move(m_p->m_elements.front());
        } else {
            value = m_p->m_elements.front();
        }

        m_p->m_elements.pop();
        return value;
    }

    return unexpected<queue_consume_result>(queue_consume_result::kTryLockFailure);
}

template<typename element_type>
silicon::scheduler::task<void> queue<element_type>::shutdown() {
    auto expected = m_p->m_running_state.load(std::memory_order::acquire);
    if(expected == running_state_t::kStopped) {
        co_return;
    }

    // We use the lock to guarantee the m_p->m_running_state has propagated.
    auto lk = co_await m_p->m_mutex.scoped_lock();
    if(!m_p->m_running_state.compare_exchange_strong(
               expected, running_state_t::kStopped, std::memory_order::acq_rel, std::memory_order::relaxed
       )) {
        co_return;
    }

    auto *waiters = m_p->m_waiters;
    m_p->m_waiters = nullptr;
    lk.unlock();
    while(waiters != nullptr) {
        auto *next = waiters->m_next;
        waiters->m_awaiting_coroutine.resume();
        waiters = next;
    }
}

template<typename element_type>
template<silicon::coroutine::concepts::executor executor_type>
silicon::scheduler::task<void> queue<element_type>::shutdown_drain(std::unique_ptr<executor_type> &e) {
    auto lk = co_await m_p->m_mutex.scoped_lock();
    auto expected = running_state_t::kRunning;
    if(!m_p->m_running_state.compare_exchange_strong(
               expected, running_state_t::kDraining, std::memory_order::acq_rel, std::memory_order::relaxed
       )) {
        co_return;
    }
    lk.unlock();

    while(!empty() && m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kDraining) {
        co_await e->yield();
    }

    co_return co_await shutdown();
}

template<typename element_type>
bool queue<element_type>::is_shutdown() const { return m_p->m_running_state.load(std::memory_order::acquire) != running_state_t::kRunning; }

} // namespace silicon::coroutine

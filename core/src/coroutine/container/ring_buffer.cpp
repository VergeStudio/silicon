module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
// 标准库头必须置于全局模块片段：接口单元全局片段中的 #include 对实现单元不可达。
#include <array>
#include <atomic>
#include <coroutine>
#include <memory>
#include <optional>
#include <utility>

module silicon.coroutine;

namespace silicon::coroutine {

// ===========================================================================
// ring_buffer::produce_operation
// ===========================================================================

template<typename element, size_t num_elements>
ring_buffer<element, num_elements>::produce_operation::produce_operation(ring_buffer<element, num_elements> &rb, element e)
    : m_rb(rb),
      m_e(std::move(e)) {}

template<typename element, size_t num_elements>
bool ring_buffer<element, num_elements>::produce_operation::await_ready() noexcept {
    auto &mutex = m_rb.m_p->m_mutex;

    // Produce operations can only proceed if running.
    if(m_rb.m_p->m_running_state.load(std::memory_order::acquire) != running_state_t::kRunning) {
        m_result = ring_buffer_result::produce::kStopped;
        static_cast<void>(mutex.unlock());
        return true; // Will be awoken with produce::stopped
    }

    if(m_rb.m_p->m_used.load(std::memory_order::acquire) < num_elements) {
        // There is guaranteed space to store
        auto slot = m_rb.m_p->m_front.fetch_add(1, std::memory_order::acq_rel) % num_elements;
        m_rb.m_p->m_elements[slot] = std::move(m_e);
        m_rb.m_p->m_used.fetch_add(1, std::memory_order::release);
        static_cast<void>(mutex.unlock());
        return true; // Will be awoken with produce::produced
    }

    return false; // ring buffer full, suspend
}

template<typename element, size_t num_elements>
bool ring_buffer<element, num_elements>::produce_operation::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
    m_awaiting_coroutine = awaiting_coroutine;
    m_next = m_rb.m_p->m_produce_waiters.exchange(this, std::memory_order::acq_rel);
    static_cast<void>(m_rb.m_p->m_mutex.unlock());
    return true;
}

template<typename element, size_t num_elements>
auto ring_buffer<element, num_elements>::produce_operation::await_resume() -> ring_buffer_result::produce {
    return m_result;
}

// ===========================================================================
// ring_buffer::consume_operation
// ===========================================================================

template<typename element, size_t num_elements>
ring_buffer<element, num_elements>::consume_operation::consume_operation(ring_buffer<element, num_elements> &rb)
    : m_rb(rb) {}

template<typename element, size_t num_elements>
bool ring_buffer<element, num_elements>::consume_operation::await_ready() noexcept {
    auto &mutex = m_rb.m_p->m_mutex;

    // Consume operations proceed until stopped.
    if(m_rb.m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped) {
        m_result = ring_buffer_result::consume::kStopped;
        static_cast<void>(mutex.unlock());
        return true;
    }

    if(m_rb.m_p->m_used.load(std::memory_order::acquire) > 0) {
        auto slot = m_rb.m_p->m_back.fetch_add(1, std::memory_order::acq_rel) % num_elements;
        m_e = std::move(m_rb.m_p->m_elements[slot]);
        m_rb.m_p->m_elements[slot] = std::nullopt;
        m_rb.m_p->m_used.fetch_sub(1, std::memory_order::release);
        static_cast<void>(mutex.unlock());
        return true;
    }

    return false; // ring buffer is empty, suspend.
}

template<typename element, size_t num_elements>
bool ring_buffer<element, num_elements>::consume_operation::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
    m_awaiting_coroutine = awaiting_coroutine;
    m_next = m_rb.m_p->m_consume_waiters.exchange(this, std::memory_order::acq_rel);
    static_cast<void>(m_rb.m_p->m_mutex.unlock());
    return true;
}

template<typename element, size_t num_elements>
auto ring_buffer<element, num_elements>::consume_operation::await_resume() -> expected<element, ring_buffer_result::consume> {
    if(m_e.has_value()) {
        return expected<element, ring_buffer_result::consume>(std::move(m_e).value());
    } else // state is stopped
    {
        return unexpected<ring_buffer_result::consume>(m_result);
    }
}

// ===========================================================================
// ring_buffer
// ===========================================================================

template<typename element, size_t num_elements>
ring_buffer<element, num_elements>::ring_buffer()
    : m_p(std::make_unique<impl>()) {
    static_assert(num_elements != 0, "num_elements cannot be zero");
}

template<typename element, size_t num_elements>
ring_buffer<element, num_elements>::~ring_buffer() {
    // Wake up anyone still using the ring buffer.
    silicon::coroutine::sync_wait(shutdown());
}

template<typename element, size_t num_elements>
silicon::scheduler::task<ring_buffer_result::produce> ring_buffer<element, num_elements>::produce(element e) {
    co_await m_p->m_mutex.lock();
    auto result = co_await produce_operation{*this, std::move(e)};
    co_await try_resume_consumers();
    co_return result;
}

template<typename element, size_t num_elements>
silicon::scheduler::task<expected<element, ring_buffer_result::consume>> ring_buffer<element, num_elements>::consume() {
    co_await m_p->m_mutex.lock();
    auto result = co_await consume_operation{*this};
    co_await try_resume_producers();
    co_return result;
}

template<typename element, size_t num_elements>
auto ring_buffer<element, num_elements>::size() const -> size_t {
    return m_p->m_used.load(std::memory_order::acquire);
}

template<typename element, size_t num_elements>
bool ring_buffer<element, num_elements>::empty() const { return size() == 0; }

template<typename element, size_t num_elements>
bool ring_buffer<element, num_elements>::full() const { return size() == max_size(); }

template<typename element, size_t num_elements>
silicon::scheduler::task<void> ring_buffer<element, num_elements>::notify_producers() {
    auto expected = m_p->m_running_state.load(std::memory_order::acquire);
    if(expected == running_state_t::kStopped) {
        co_return;
    }

    co_await m_p->m_mutex.lock();
    auto *produce_waiters = m_p->m_produce_waiters.exchange(nullptr, std::memory_order::acq_rel);
    static_cast<void>(m_p->m_mutex.unlock());

    while(produce_waiters != nullptr) {
        auto *next = produce_waiters->m_next;
        produce_waiters->m_result = ring_buffer_result::produce::kNotified;
        produce_waiters->m_awaiting_coroutine.resume();
        produce_waiters = next;
    }

    co_return;
}

template<typename element, size_t num_elements>
silicon::scheduler::task<void> ring_buffer<element, num_elements>::notify_consumers() {
    auto expected = m_p->m_running_state.load(std::memory_order::acquire);
    if(expected == running_state_t::kStopped) {
        co_return;
    }

    co_await m_p->m_mutex.lock();
    auto *consume_waiters = m_p->m_consume_waiters.exchange(nullptr, std::memory_order::acq_rel);
    static_cast<void>(m_p->m_mutex.unlock());

    while(consume_waiters != nullptr) {
        auto *next = consume_waiters->m_next;
        consume_waiters->m_result = ring_buffer_result::consume::kNotified;
        consume_waiters->m_awaiting_coroutine.resume();
        consume_waiters = next;
    }

    co_return;
}

template<typename element, size_t num_elements>
silicon::scheduler::task<void> ring_buffer<element, num_elements>::shutdown() {
    // Only wake up waiters once.
    auto expected = m_p->m_running_state.load(std::memory_order::acquire);
    if(expected == running_state_t::kStopped) {
        co_return;
    }

    auto lk = co_await m_p->m_mutex.scoped_lock();
    // Only let one caller do the wake-ups, this can go from running or draining to stopped
    if(!m_p->m_running_state.compare_exchange_strong(expected, running_state_t::kStopped, std::memory_order::acq_rel, std::memory_order::relaxed)) {
        co_return;
    }
    lk.unlock();

    co_await m_p->m_mutex.lock();
    auto *produce_waiters = m_p->m_produce_waiters.exchange(nullptr, std::memory_order::acq_rel);
    auto *consume_waiters = m_p->m_consume_waiters.exchange(nullptr, std::memory_order::acq_rel);
    static_cast<void>(m_p->m_mutex.unlock());

    while(produce_waiters != nullptr) {
        auto *next = produce_waiters->m_next;
        produce_waiters->m_result = ring_buffer_result::produce::kStopped;
        produce_waiters->m_awaiting_coroutine.resume();
        produce_waiters = next;
    }

    while(consume_waiters != nullptr) {
        auto *next = consume_waiters->m_next;
        consume_waiters->m_result = ring_buffer_result::consume::kStopped;
        consume_waiters->m_awaiting_coroutine.resume();
        consume_waiters = next;
    }

    co_return;
}

template<typename element, size_t num_elements>
template<silicon::coroutine::concepts::executor executor_type>
silicon::scheduler::task<void> ring_buffer<element, num_elements>::shutdown_drain(std::unique_ptr<executor_type> &e) {
    auto lk = co_await m_p->m_mutex.scoped_lock();
    // Do not allow any more produces, the state must be in running to drain.
    auto expected = running_state_t::kRunning;
    if(!m_p->m_running_state.compare_exchange_strong(expected, running_state_t::kDraining, std::memory_order::acq_rel, std::memory_order::relaxed)) {
        co_return;
    }

    auto *produce_waiters = m_p->m_produce_waiters.exchange(nullptr, std::memory_order::acq_rel);
    lk.unlock();

    while(produce_waiters != nullptr) {
        auto *next = produce_waiters->m_next;
        produce_waiters->m_awaiting_coroutine.resume();
        produce_waiters = next;
    }

    while(!empty() && m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kDraining) {
        co_await e->yield();
    }

    co_await shutdown();
    co_return;
}

template<typename element, size_t num_elements>
bool ring_buffer<element, num_elements>::is_shutdown() const { return m_p->m_running_state.load(std::memory_order::acquire) != running_state_t::kRunning; }

// ===========================================================================
// ring_buffer internal resume helpers
// ===========================================================================

template<typename element, size_t num_elements>
silicon::scheduler::task<void> ring_buffer<element, num_elements>::try_resume_producers() {
    while(true) {
        auto lk = co_await m_p->m_mutex.scoped_lock();
        if(m_p->m_used.load(std::memory_order::acquire) < num_elements) {
            auto *op = awaiter_list_pop(m_p->m_produce_waiters);
            if(op != nullptr) {
                auto slot = m_p->m_front.fetch_add(1, std::memory_order::acq_rel) % num_elements;
                m_p->m_elements[slot] = std::move(op->m_e);
                m_p->m_used.fetch_add(1, std::memory_order::release);

                lk.unlock();
                op->m_awaiting_coroutine.resume();
                continue;
            }
        }
        co_return;
    }
}

template<typename element, size_t num_elements>
silicon::scheduler::task<void> ring_buffer<element, num_elements>::try_resume_consumers() {
    while(true) {
        auto lk = co_await m_p->m_mutex.scoped_lock();
        if(m_p->m_used.load(std::memory_order::acquire) > 0) {
            auto *op = awaiter_list_pop(m_p->m_consume_waiters);
            if(op != nullptr) {
                auto slot = m_p->m_back.fetch_add(1, std::memory_order::acq_rel) % num_elements;
                op->m_e = std::move(m_p->m_elements[slot]);
                m_p->m_elements[slot] = std::nullopt;
                m_p->m_used.fetch_sub(1, std::memory_order::release);
                lk.unlock();

                op->m_awaiting_coroutine.resume();
                continue;
            }
        }
        co_return;
    }
}

} // namespace silicon::coroutine

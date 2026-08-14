module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
// 标准库头必须置于全局模块片段：接口单元全局片段中的 #include 对实现单元不可达。
#include <atomic>
#include <coroutine>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

module silicon.coroutine;

namespace silicon::coroutine {

// ===========================================================================
// channel::send_operation
// ===========================================================================

template<typename element_type>
channel<element_type>::send_operation::send_operation(channel<element_type> &ch, element_type e) noexcept
    : m_ch(ch),
      m_e(std::move(e)) {}

template<typename element_type>
auto channel<element_type>::send_operation::await_ready() noexcept -> bool {
    auto &mutex = m_ch.m_p->m_mutex;

    // Sends are rejected once the channel has been closed.
    if(m_ch.m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped) {
        m_result = channel_result::send::kClosed;
        mutex.unlock();
        return true;
    }

    // Hand the element directly to a waiting receiver (rendezvous).
    if(auto *waiter = m_ch.m_p->pop_recv_waiter()) {
        waiter->m_e = std::move(m_e);
        mutex.unlock();
        waiter->m_awaiting_coroutine.resume();
        return true;
    }

    // Store the element into a free slot when the buffer has room.
    if(m_ch.m_p->m_count.load(std::memory_order::acquire) < m_ch.m_p->m_capacity) {
        m_ch.m_p->store(std::move(m_e).value());
        mutex.unlock();
        return true;
    }

    // The channel is full, suspend until a slot is freed.
    return false;
}

template<typename element_type>
auto channel<element_type>::send_operation::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
    m_awaiting_coroutine = awaiting_coroutine;
    m_ch.m_p->append_send_waiter(this);
    m_ch.m_p->m_mutex.unlock();
    return true;
}

template<typename element_type>
auto channel<element_type>::send_operation::await_resume() noexcept -> channel_result::send { return m_result; }

// ===========================================================================
// channel::recv_operation
// ===========================================================================

template<typename element_type>
channel<element_type>::recv_operation::recv_operation(channel<element_type> &ch) noexcept
    : m_ch(ch) {}

template<typename element_type>
auto channel<element_type>::recv_operation::await_ready() noexcept -> bool {
    auto &mutex = m_ch.m_p->m_mutex;

    // Take a buffered element first.
    if(m_ch.m_p->m_count.load(std::memory_order::acquire) > 0) {
        m_e = m_ch.m_p->take();
        mutex.unlock();
        return true;
    }

    // Rendezvous with a waiting producer (unbuffered case).
    if(auto *waiter = m_ch.m_p->pop_send_waiter()) {
        m_e = std::move(waiter->m_e);
        mutex.unlock();
        waiter->m_awaiting_coroutine.resume();
        return true;
    }

    // The channel is closed and drained.
    if(m_ch.m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped) {
        m_result = channel_result::recv::kClosed;
        mutex.unlock();
        return true;
    }

    // The channel is empty, suspend until an element is produced.
    return false;
}

template<typename element_type>
auto channel<element_type>::recv_operation::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
    m_awaiting_coroutine = awaiting_coroutine;
    m_ch.m_p->append_recv_waiter(this);
    m_ch.m_p->m_mutex.unlock();
    return true;
}

template<typename element_type>
auto channel<element_type>::recv_operation::await_resume() noexcept -> expected<element_type, channel_result::recv> {
    if(m_e.has_value()) {
        return expected<element_type, channel_result::recv>(std::move(m_e).value());
    }
    return unexpected<channel_result::recv>(m_result);
}

// ===========================================================================
// channel
// ===========================================================================

template<typename element_type>
channel<element_type>::channel(size_t capacity)
    : m_p(std::make_unique<impl>(capacity)) {}

template<typename element_type>
channel<element_type>::~channel() {
    // Non-blocking wake-up of all waiters, mirroring queue::~queue.
    // Blocking here (e.g. sync_wait(shutdown())) would deadlock when the
    // channel is destroyed from within a coroutine context, because the
    // scheduler would never get to process the shutdown coroutine.
    if(m_p->m_running_state.exchange(running_state_t::kStopped, std::memory_order::acq_rel) == running_state_t::kStopped) {
        return;
    }

    auto *send_waiters = m_p->m_send_waiters_head;
    auto *recv_waiters = m_p->m_recv_waiters_head;
    m_p->m_send_waiters_head = m_p->m_send_waiters_tail = nullptr;
    m_p->m_recv_waiters_head = m_p->m_recv_waiters_tail = nullptr;

    while(send_waiters != nullptr) {
        auto *next = send_waiters->m_next;
        send_waiters->m_result = channel_result::send::kClosed;
        send_waiters->m_awaiting_coroutine.resume();
        send_waiters = next;
    }

    while(recv_waiters != nullptr) {
        auto *next = recv_waiters->m_next;
        recv_waiters->m_result = channel_result::recv::kClosed;
        recv_waiters->m_awaiting_coroutine.resume();
        recv_waiters = next;
    }
}

template<typename element_type>
auto channel<element_type>::send(const element_type &element) -> silicon::scheduler::task<channel_result::send> {
    co_await m_p->m_mutex.lock();
    auto result = co_await send_operation{*this, element};
    co_await try_resume_receivers();
    co_return result;
}

template<typename element_type>
auto channel<element_type>::send(element_type &&element) -> silicon::scheduler::task<channel_result::send> {
    co_await m_p->m_mutex.lock();
    auto result = co_await send_operation{*this, std::move(element)};
    co_await try_resume_receivers();
    co_return result;
}

template<typename element_type>
auto channel<element_type>::try_send(const element_type &element) -> channel_result::send {
    if(!m_p->m_mutex.try_lock()) {
        return channel_result::send::kFull;
    }
    return do_try_send(element);
}

template<typename element_type>
auto channel<element_type>::try_send(element_type &&element) -> channel_result::send {
    if(!m_p->m_mutex.try_lock()) {
        return channel_result::send::kFull;
    }
    return do_try_send(std::move(element));
}

template<typename element_type>
auto channel<element_type>::recv() -> silicon::scheduler::task<expected<element_type, channel_result::recv>> {
    co_await m_p->m_mutex.lock();
    auto result = co_await recv_operation{*this};
    co_await try_resume_senders();
    co_return result;
}

template<typename element_type>
auto channel<element_type>::try_recv() -> expected<element_type, channel_result::recv> {
    if(!m_p->m_mutex.try_lock()) {
        return unexpected<channel_result::recv>(channel_result::recv::kEmpty);
    }

    if(m_p->m_count.load(std::memory_order::acquire) > 0) {
        auto element = m_p->take();
        m_p->m_mutex.unlock();
        return expected<element_type, channel_result::recv>(std::move(element).value());
    }

    if(auto *waiter = m_p->pop_send_waiter()) {
        auto element = std::move(waiter->m_e);
        m_p->m_mutex.unlock();
        waiter->m_awaiting_coroutine.resume();
        return expected<element_type, channel_result::recv>(std::move(element).value());
    }

    auto result = m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped
                          ? channel_result::recv::kClosed
                          : channel_result::recv::kEmpty;
    m_p->m_mutex.unlock();
    return unexpected<channel_result::recv>(result);
}

template<typename element_type>
auto channel<element_type>::close() -> silicon::scheduler::task<void> {
    auto expected_state = m_p->m_running_state.load(std::memory_order::acquire);
    if(expected_state == running_state_t::kStopped) {
        co_return;
    }

    auto lk = co_await m_p->m_mutex.scoped_lock();
    if(!m_p->m_running_state.compare_exchange_strong(
               expected_state, running_state_t::kStopped, std::memory_order::acq_rel, std::memory_order::relaxed
       )) {
        co_return;
    }

    auto *send_waiters = m_p->m_send_waiters_head;
    auto *recv_waiters = m_p->m_recv_waiters_head;
    m_p->m_send_waiters_head = m_p->m_send_waiters_tail = nullptr;
    m_p->m_recv_waiters_head = m_p->m_recv_waiters_tail = nullptr;
    lk.unlock();

    while(send_waiters != nullptr) {
        auto *next = send_waiters->m_next;
        send_waiters->m_result = channel_result::send::kClosed;
        send_waiters->m_awaiting_coroutine.resume();
        send_waiters = next;
    }

    while(recv_waiters != nullptr) {
        auto *next = recv_waiters->m_next;
        recv_waiters->m_result = channel_result::recv::kClosed;
        recv_waiters->m_awaiting_coroutine.resume();
        recv_waiters = next;
    }
}

template<typename element_type>
auto channel<element_type>::closed() const -> bool { return m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped; }

template<typename element_type>
auto channel<element_type>::capacity() const -> size_t { return m_p->m_capacity; }

template<typename element_type>
auto channel<element_type>::size() const -> size_t { return m_p->m_count.load(std::memory_order::acquire); }

template<typename element_type>
auto channel<element_type>::empty() const -> bool { return size() == 0; }

template<typename element_type>
auto channel<element_type>::full() const -> bool { return size() >= m_p->m_capacity; }

template<typename element_type>
auto channel<element_type>::do_try_send(element_type element) -> channel_result::send {
    if(m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped) {
        m_p->m_mutex.unlock();
        return channel_result::send::kClosed;
    }

    if(auto *waiter = m_p->pop_recv_waiter()) {
        waiter->m_e = std::move(element);
        m_p->m_mutex.unlock();
        waiter->m_awaiting_coroutine.resume();
        return channel_result::send::kSent;
    }

    if(m_p->m_count.load(std::memory_order::acquire) < m_p->m_capacity) {
        m_p->store(std::move(element));
        m_p->m_mutex.unlock();
        return channel_result::send::kSent;
    }

    m_p->m_mutex.unlock();
    return channel_result::send::kFull;
}

template<typename element_type>
auto channel<element_type>::try_resume_senders() -> silicon::scheduler::task<void> {
    while(true) {
        auto lk = co_await m_p->m_mutex.scoped_lock();
        if(m_p->m_count.load(std::memory_order::acquire) < m_p->m_capacity) {
            auto *op = m_p->pop_send_waiter();
            if(op != nullptr) {
                m_p->store(std::move(op->m_e).value());
                lk.unlock();
                op->m_awaiting_coroutine.resume();
                continue;
            }
        }
        co_return;
    }
}

template<typename element_type>
auto channel<element_type>::try_resume_receivers() -> silicon::scheduler::task<void> {
    while(true) {
        auto lk = co_await m_p->m_mutex.scoped_lock();
        if(m_p->m_count.load(std::memory_order::acquire) > 0) {
            auto *op = m_p->pop_recv_waiter();
            if(op != nullptr) {
                op->m_e = m_p->take();
                lk.unlock();
                op->m_awaiting_coroutine.resume();
                continue;
            }
        }
        co_return;
    }
}

// ===========================================================================
// channel::impl
// ===========================================================================

template<typename element_type>
channel<element_type>::impl::impl(size_t capacity)
    : m_capacity(capacity),
      m_slots(capacity) {}

template<typename element_type>
auto channel<element_type>::impl::store(element_type &&element) -> void {
    m_slots[m_tail] = std::move(element);
    m_tail = (m_tail + 1) % m_capacity;
    m_count.fetch_add(1, std::memory_order::release);
}

template<typename element_type>
auto channel<element_type>::impl::take() -> std::optional<element_type> {
    auto element = std::move(m_slots[m_head]);
    m_slots[m_head].reset();
    m_head = (m_head + 1) % m_capacity;
    m_count.fetch_sub(1, std::memory_order::release);
    return element;
}

template<typename element_type>
auto channel<element_type>::impl::append_send_waiter(send_operation *op) -> void {
    op->m_next = nullptr;
    if(m_send_waiters_tail != nullptr) {
        m_send_waiters_tail->m_next = op;
    } else {
        m_send_waiters_head = op;
    }
    m_send_waiters_tail = op;
}

template<typename element_type>
auto channel<element_type>::impl::pop_send_waiter() -> send_operation * {
    auto *op = m_send_waiters_head;
    if(op != nullptr) {
        m_send_waiters_head = op->m_next;
        if(m_send_waiters_head == nullptr) {
            m_send_waiters_tail = nullptr;
        }
        op->m_next = nullptr;
    }
    return op;
}

template<typename element_type>
auto channel<element_type>::impl::append_recv_waiter(recv_operation *op) -> void {
    op->m_next = nullptr;
    if(m_recv_waiters_tail != nullptr) {
        m_recv_waiters_tail->m_next = op;
    } else {
        m_recv_waiters_head = op;
    }
    m_recv_waiters_tail = op;
}

template<typename element_type>
auto channel<element_type>::impl::pop_recv_waiter() -> recv_operation * {
    auto *op = m_recv_waiters_head;
    if(op != nullptr) {
        m_recv_waiters_head = op->m_next;
        if(m_recv_waiters_head == nullptr) {
            m_recv_waiters_tail = nullptr;
        }
        op->m_next = nullptr;
    }
    return op;
}

} // namespace silicon::coroutine

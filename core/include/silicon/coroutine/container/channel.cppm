module;

#include <expected>

#include <atomic>
#include <coroutine>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

export module silicon.coroutine:channel;

import silicon.scheduler;
import :mutex;
import silicon.scheduler.task;
export namespace silicon::coroutine {

namespace channel_result {
enum class send {
    kSent,
    kFull,
    kClosed,
};

enum class recv {
    kEmpty,
    kClosed,
};
}

template<typename element_type>
class channel {
  private:
    enum class running_state_t {
        kRunning,
        kStopped,
    };

    struct impl;

  public:
    struct send_operation {
        send_operation(channel<element_type> &, element_type) noexcept;

        bool await_ready() noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;
        auto await_resume() noexcept -> channel_result::send;

        std::coroutine_handle<> m_awaiting_coroutine{nullptr};
        channel_result::send m_result{channel_result::send::kSent};
        send_operation *m_next{nullptr};

      private:
        friend class channel;

        channel<element_type> &m_ch;
        std::optional<element_type> m_e;
    };

    struct recv_operation {
        explicit recv_operation(channel<element_type> &) noexcept;

        bool await_ready() noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;
        auto await_resume() noexcept -> std::expected<element_type, channel_result::recv>;

        std::coroutine_handle<> m_awaiting_coroutine{nullptr};
        channel_result::recv m_result{channel_result::recv::kClosed};
        recv_operation *m_next{nullptr};

      private:
        friend class channel;

        channel<element_type> &m_ch;
        std::optional<element_type> m_e;
    };

    explicit channel(size_t);

    ~channel();

    channel(const channel &) = delete;
    channel(channel &&) = delete;
    channel & operator=(const channel &) = delete;
    channel & operator=(channel &&) = delete;

    silicon::scheduler::task<channel_result::send> send(const element_type &) ;

    silicon::scheduler::task<channel_result::send> send(element_type &&element) ;

    auto try_send(const element_type &) -> channel_result::send;

    auto try_send(element_type &&element) -> channel_result::send;

    [[nodiscard]] silicon::scheduler::task<std::expected<element_type, channel_result::recv>> recv() ;

    [[nodiscard]] auto try_recv() -> std::expected<element_type, channel_result::recv>;

    silicon::scheduler::task<void> close() ;

    [[nodiscard]] bool closed() const ;

    [[nodiscard]] size_t capacity() const ;

    [[nodiscard]] size_t size() const ;

    [[nodiscard]] bool empty() const ;

    [[nodiscard]] bool full() const ;

  private:
    friend send_operation;
    friend recv_operation;

    auto do_try_send(element_type) -> channel_result::send;
    silicon::scheduler::task<void> try_resume_senders() ;
    silicon::scheduler::task<void> try_resume_receivers() ;

    struct impl {
      public:
        explicit impl(size_t);

        void store(element_type &&element) ;
        std::optional<element_type> take() ;
        void append_send_waiter(send_operation *) ;
        send_operation * pop_send_waiter() ;
        void append_recv_waiter(recv_operation *) ;
        recv_operation * pop_recv_waiter() ;

        silicon::coroutine::mutex m_mutex{};
        size_t m_capacity{0};
        std::vector<std::optional<element_type>> m_slots{};
        size_t m_head{0};
        size_t m_tail{0};
        std::atomic<size_t> m_count{0};
        std::atomic<running_state_t> m_running_state{running_state_t::kRunning};
        send_operation *m_send_waiters_head{nullptr};
        send_operation *m_send_waiters_tail{nullptr};
        recv_operation *m_recv_waiters_head{nullptr};
        recv_operation *m_recv_waiters_tail{nullptr};
    };

    std::unique_ptr<impl> m_p;
};

template<typename element_type>
channel<element_type>::send_operation::send_operation(channel<element_type> &ch, element_type e) noexcept
    : m_ch(ch),
      m_e(std::move(e)) {}

template<typename element_type>
bool channel<element_type>::send_operation::await_ready() noexcept {
    auto &mutex = m_ch.m_p->m_mutex;

    if(m_ch.m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped) {
        m_result = channel_result::send::kClosed;
        static_cast<void>(mutex.unlock());
        return true;
    }

    if(auto *waiter = m_ch.m_p->pop_recv_waiter()) {
        waiter->m_e = std::move(m_e);
        static_cast<void>(mutex.unlock());
        waiter->m_awaiting_coroutine.resume();
        return true;
    }

    if(m_ch.m_p->m_count.load(std::memory_order::acquire) < m_ch.m_p->m_capacity) {
        m_ch.m_p->store(std::move(m_e).value());
        static_cast<void>(mutex.unlock());
        return true;
    }

    return false;
}

template<typename element_type>
bool channel<element_type>::send_operation::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
    m_awaiting_coroutine = awaiting_coroutine;
    m_ch.m_p->append_send_waiter(this);
    static_cast<void>(m_ch.m_p->m_mutex.unlock());
    return true;
}

template<typename element_type>
auto channel<element_type>::send_operation::await_resume() noexcept -> channel_result::send { return m_result; }

template<typename element_type>
channel<element_type>::recv_operation::recv_operation(channel<element_type> &ch) noexcept
    : m_ch(ch) {}

template<typename element_type>
bool channel<element_type>::recv_operation::await_ready() noexcept {
    auto &mutex = m_ch.m_p->m_mutex;

    if(m_ch.m_p->m_count.load(std::memory_order::acquire) > 0) {
        m_e = m_ch.m_p->take();
        static_cast<void>(mutex.unlock());
        return true;
    }

    if(auto *waiter = m_ch.m_p->pop_send_waiter()) {
        m_e = std::move(waiter->m_e);
        static_cast<void>(mutex.unlock());
        waiter->m_awaiting_coroutine.resume();
        return true;
    }

    if(m_ch.m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped) {
        m_result = channel_result::recv::kClosed;
        static_cast<void>(mutex.unlock());
        return true;
    }

    return false;
}

template<typename element_type>
bool channel<element_type>::recv_operation::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
    m_awaiting_coroutine = awaiting_coroutine;
    m_ch.m_p->append_recv_waiter(this);
    static_cast<void>(m_ch.m_p->m_mutex.unlock());
    return true;
}

template<typename element_type>
auto channel<element_type>::recv_operation::await_resume() noexcept -> std::expected<element_type, channel_result::recv> {
    if(m_e.has_value()) {
        return std::expected<element_type, channel_result::recv>(std::move(m_e).value());
    }
    return std::unexpected<channel_result::recv>(m_result);
}

template<typename element_type>
channel<element_type>::channel(size_t capacity)
    : m_p(std::make_unique<impl>(capacity)) {}

template<typename element_type>
channel<element_type>::~channel() {

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
silicon::scheduler::task<channel_result::send> channel<element_type>::send(const element_type &element) {
    co_await m_p->m_mutex.lock();
    auto result = co_await send_operation{*this, element};
    co_await try_resume_receivers();
    co_return result;
}

template<typename element_type>
silicon::scheduler::task<channel_result::send> channel<element_type>::send(element_type &&element) {
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
silicon::scheduler::task<std::expected<element_type, channel_result::recv>> channel<element_type>::recv() {
    co_await m_p->m_mutex.lock();
    auto result = co_await recv_operation{*this};
    co_await try_resume_senders();
    co_return result;
}

template<typename element_type>
auto channel<element_type>::try_recv() -> std::expected<element_type, channel_result::recv> {
    if(!m_p->m_mutex.try_lock()) {
        return std::unexpected<channel_result::recv>(channel_result::recv::kEmpty);
    }

    if(m_p->m_count.load(std::memory_order::acquire) > 0) {
        auto element = m_p->take();
        static_cast<void>(m_p->m_mutex.unlock());
        return std::expected<element_type, channel_result::recv>(std::move(element).value());
    }

    if(auto *waiter = m_p->pop_send_waiter()) {
        auto element = std::move(waiter->m_e);
        static_cast<void>(m_p->m_mutex.unlock());
        waiter->m_awaiting_coroutine.resume();
        return std::expected<element_type, channel_result::recv>(std::move(element).value());
    }

    auto result = m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped
                          ? channel_result::recv::kClosed
                          : channel_result::recv::kEmpty;
    static_cast<void>(m_p->m_mutex.unlock());
    return std::unexpected<channel_result::recv>(result);
}

template<typename element_type>
silicon::scheduler::task<void> channel<element_type>::close() {
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
bool channel<element_type>::closed() const { return m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped; }

template<typename element_type>
auto channel<element_type>::capacity() const -> size_t { return m_p->m_capacity; }

template<typename element_type>
auto channel<element_type>::size() const -> size_t { return m_p->m_count.load(std::memory_order::acquire); }

template<typename element_type>
bool channel<element_type>::empty() const { return size() == 0; }

template<typename element_type>
bool channel<element_type>::full() const { return size() >= m_p->m_capacity; }

template<typename element_type>
auto channel<element_type>::do_try_send(element_type element) -> channel_result::send {
    if(m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped) {
        static_cast<void>(m_p->m_mutex.unlock());
        return channel_result::send::kClosed;
    }

    if(auto *waiter = m_p->pop_recv_waiter()) {
        waiter->m_e = std::move(element);
        static_cast<void>(m_p->m_mutex.unlock());
        waiter->m_awaiting_coroutine.resume();
        return channel_result::send::kSent;
    }

    if(m_p->m_count.load(std::memory_order::acquire) < m_p->m_capacity) {
        m_p->store(std::move(element));
        static_cast<void>(m_p->m_mutex.unlock());
        return channel_result::send::kSent;
    }

    static_cast<void>(m_p->m_mutex.unlock());
    return channel_result::send::kFull;
}

template<typename element_type>
silicon::scheduler::task<void> channel<element_type>::try_resume_senders() {
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
silicon::scheduler::task<void> channel<element_type>::try_resume_receivers() {
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

template<typename element_type>
channel<element_type>::impl::impl(size_t capacity)
    : m_capacity(capacity),
      m_slots(capacity) {}

template<typename element_type>
void channel<element_type>::impl::store(element_type &&element) {
    m_slots[m_tail] = std::move(element);
    m_tail = (m_tail + 1) % m_capacity;
    m_count.fetch_add(1, std::memory_order::release);
}

template<typename element_type>
std::optional<element_type> channel<element_type>::impl::take() {
    auto element = std::move(m_slots[m_head]);
    m_slots[m_head].reset();
    m_head = (m_head + 1) % m_capacity;
    m_count.fetch_sub(1, std::memory_order::release);
    return element;
}

template<typename element_type>
void channel<element_type>::impl::append_send_waiter(send_operation *op) {
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
void channel<element_type>::impl::append_recv_waiter(recv_operation *op) {
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

}

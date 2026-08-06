module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <atomic>
#include <coroutine>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

export module silicon.coroutine:channel;

import :expected;
import :mutex;
import :task;

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
} // namespace channel_result

/**
 * @brief A multi-producer / multi-consumer communication channel.
 *
 * The channel is bounded by a user supplied capacity. When the capacity is
 * exhausted a producer suspends until a consumer frees a slot; when the
 * channel is empty a consumer suspends until a producer delivers an element.
 * With capacity 0 the channel behaves like a synchronous rendezvous: send()
 * suspends until a receiver is waiting and the element is handed off directly
 * without touching a buffer slot.
 *
 * Waiting producers and consumers are woken up in FIFO order (fairness).
 *
 * close() is idempotent. After close(), send() returns
 * channel_result::send::kClosed and recv() keeps returning the elements that
 * are still buffered before it reports channel_result::recv::kClosed (drain
 * semantics, like Go channels). All suspended waiters are woken up with the
 * closed result.
 *
 * @tparam element_type The type of items being sent and received.
 */
template<typename element_type>
class channel {
  private:
    enum class running_state_t {
        kRunning,
        kStopped,
    };

    struct Impl;

  public:
    struct send_operation {
        send_operation(channel<element_type> &ch, element_type e) noexcept
            : m_ch(ch),
              m_e(std::move(e)) {}

        auto await_ready() noexcept -> bool {
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

        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
            m_awaiting_coroutine = awaiting_coroutine;
            m_ch.m_p->append_send_waiter(this);
            m_ch.m_p->m_mutex.unlock();
            return true;
        }

        auto await_resume() noexcept -> channel_result::send { return m_result; }

        std::coroutine_handle<> m_awaiting_coroutine{nullptr};
        channel_result::send m_result{channel_result::send::kSent};
        send_operation *m_next{nullptr};

      private:
        friend class channel;

        channel<element_type> &m_ch;
        std::optional<element_type> m_e;
    };

    struct recv_operation {
        explicit recv_operation(channel<element_type> &ch) noexcept
            : m_ch(ch) {}

        auto await_ready() noexcept -> bool {
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

        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool {
            m_awaiting_coroutine = awaiting_coroutine;
            m_ch.m_p->append_recv_waiter(this);
            m_ch.m_p->m_mutex.unlock();
            return true;
        }

        auto await_resume() noexcept -> expected<element_type, channel_result::recv> {
            if(m_e.has_value()) {
                return expected<element_type, channel_result::recv>(std::move(m_e).value());
            }
            return unexpected<channel_result::recv>(m_result);
        }

        std::coroutine_handle<> m_awaiting_coroutine{nullptr};
        channel_result::recv m_result{channel_result::recv::kClosed};
        recv_operation *m_next{nullptr};

      private:
        friend class channel;

        channel<element_type> &m_ch;
        std::optional<element_type> m_e;
    };

    /**
     * @param capacity The maximum number of buffered elements. 0 creates an
     *                 unbuffered rendezvous channel where send() suspends
     *                 until a receiver is waiting.
     */
    explicit channel(size_t capacity)
        : m_p(std::make_unique<Impl>(capacity)) {}

    ~channel() {
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

    channel(const channel &) = delete;
    channel(channel &&) = delete;
    auto operator=(const channel &) -> channel & = delete;
    auto operator=(channel &&) -> channel & = delete;

    /**
     * @brief Sends an element into the channel, suspending until a slot is
     *        available or the element can be handed off to a waiting receiver.
     *
     * @return channel_result::send::kSent on success, or kClosed if the
     *         channel has been closed.
     */
    auto send(const element_type &element) -> silicon::coroutine::task<channel_result::send> {
        co_await m_p->m_mutex.lock();
        auto result = co_await send_operation{*this, element};
        co_await try_resume_receivers();
        co_return result;
    }

    /**
     * @brief Sends an element into the channel, suspending until a slot is
     *        available or the element can be handed off to a waiting receiver.
     *
     * @return channel_result::send::kSent on success, or kClosed if the
     *         channel has been closed.
     */
    auto send(element_type &&element) -> silicon::coroutine::task<channel_result::send> {
        co_await m_p->m_mutex.lock();
        auto result = co_await send_operation{*this, std::move(element)};
        co_await try_resume_receivers();
        co_return result;
    }

    /**
     * @brief Non-blocking send. Does not suspend.
     *
     * @return channel_result::send::kSent on success, kFull when the channel
     *         has no free slot (mutex contention is treated as kFull, like
     *         tokio::sync::mpsc), and kClosed if the channel is closed.
     */
    auto try_send(const element_type &element) -> channel_result::send {
        if(!m_p->m_mutex.try_lock()) {
            return channel_result::send::kFull;
        }
        return do_try_send(element);
    }

    /**
     * @brief Non-blocking send. Does not suspend.
     *
     * @return channel_result::send::kSent on success, kFull when the channel
     *         has no free slot (mutex contention is treated as kFull, like
     *         tokio::sync::mpsc), and kClosed if the channel is closed.
     */
    auto try_send(element_type &&element) -> channel_result::send {
        if(!m_p->m_mutex.try_lock()) {
            return channel_result::send::kFull;
        }
        return do_try_send(std::move(element));
    }

    /**
     * @brief Receives an element from the channel, suspending until an element
     *        is available or the channel is closed and drained.
     *
     * @return The element, or channel_result::recv::kClosed if the channel is
     *         closed and no buffered elements remain.
     */
    [[nodiscard]] auto recv() -> silicon::coroutine::task<expected<element_type, channel_result::recv>> {
        co_await m_p->m_mutex.lock();
        auto result = co_await recv_operation{*this};
        co_await try_resume_senders();
        co_return result;
    }

    /**
     * @brief Non-blocking receive. Does not suspend.
     *
     * @return The element, channel_result::recv::kEmpty when the channel has
     *         no element (mutex contention is treated as kEmpty, like
     *         tokio::sync::mpsc), or kClosed if the channel is closed and
     *         drained.
     */
    [[nodiscard]] auto try_recv() -> expected<element_type, channel_result::recv> {
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

    /**
     * @brief Closes the channel. Idempotent. Buffered elements are preserved
     *        and can still be received (drain semantics), then recv() reports
     *        kClosed. All suspended waiters are woken up with the closed result.
     */
    auto close() -> silicon::coroutine::task<void> {
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

    /**
     * @return true if close() has been called.
     */
    [[nodiscard]] auto closed() const -> bool { return m_p->m_running_state.load(std::memory_order::acquire) == running_state_t::kStopped; }

    /**
     * @return The maximum number of buffered elements (0 = unbuffered).
     */
    [[nodiscard]] auto capacity() const -> size_t { return m_p->m_capacity; }

    /**
     * @return The number of elements currently buffered.
     */
    [[nodiscard]] auto size() const -> size_t { return m_p->m_count.load(std::memory_order::acquire); }

    /**
     * @return true if the channel currently buffers zero elements.
     */
    [[nodiscard]] auto empty() const -> bool { return size() == 0; }

    /**
     * @return true if the channel buffer has no free slot (always true for an
     *         unbuffered channel with capacity 0).
     */
    [[nodiscard]] auto full() const -> bool { return size() >= m_p->m_capacity; }

  private:
    friend send_operation;
    friend recv_operation;

    auto do_try_send(element_type element) -> channel_result::send {
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

    auto try_resume_senders() -> silicon::coroutine::task<void> {
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

    auto try_resume_receivers() -> silicon::coroutine::task<void> {
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

    struct Impl {
      public:
        explicit Impl(size_t capacity)
            : m_capacity(capacity),
              m_slots(capacity) {}

        auto store(element_type &&element) -> void {
            m_slots[m_tail] = std::move(element);
            m_tail = (m_tail + 1) % m_capacity;
            m_count.fetch_add(1, std::memory_order::release);
        }

        auto take() -> std::optional<element_type> {
            auto element = std::move(m_slots[m_head]);
            m_slots[m_head].reset();
            m_head = (m_head + 1) % m_capacity;
            m_count.fetch_sub(1, std::memory_order::release);
            return element;
        }

        auto append_send_waiter(send_operation *op) -> void {
            op->m_next = nullptr;
            if(m_send_waiters_tail != nullptr) {
                m_send_waiters_tail->m_next = op;
            } else {
                m_send_waiters_head = op;
            }
            m_send_waiters_tail = op;
        }

        auto pop_send_waiter() -> send_operation * {
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

        auto append_recv_waiter(recv_operation *op) -> void {
            op->m_next = nullptr;
            if(m_recv_waiters_tail != nullptr) {
                m_recv_waiters_tail->m_next = op;
            } else {
                m_recv_waiters_head = op;
            }
            m_recv_waiters_tail = op;
        }

        auto pop_recv_waiter() -> recv_operation * {
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

    std::unique_ptr<Impl> m_p;
};

} // namespace silicon::coroutine

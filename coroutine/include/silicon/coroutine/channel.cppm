module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
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
 *
 * @note 实现（成员函数体）位于 coroutine/src/channel.cpp（主模块实现单元），
 *       本接口分区仅保留声明与嵌套类型布局，以降低编辑实现时的重编波及面。
 */
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
        send_operation(channel<element_type> &ch, element_type e) noexcept;

        auto await_ready() noexcept -> bool;
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool;
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
        explicit recv_operation(channel<element_type> &ch) noexcept;

        auto await_ready() noexcept -> bool;
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool;
        auto await_resume() noexcept -> expected<element_type, channel_result::recv>;

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
    explicit channel(size_t capacity);

    ~channel();

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
    auto send(const element_type &element) -> silicon::scheduler::task<channel_result::send>;

    /**
     * @brief Sends an element into the channel, suspending until a slot is
     *        available or the element can be handed off to a waiting receiver.
     *
     * @return channel_result::send::kSent on success, or kClosed if the
     *         channel has been closed.
     */
    auto send(element_type &&element) -> silicon::scheduler::task<channel_result::send>;

    /**
     * @brief Non-blocking send. Does not suspend.
     *
     * @return channel_result::send::kSent on success, kFull when the channel
     *         has no free slot (mutex contention is treated as kFull, like
     *         tokio::sync::mpsc), and kClosed if the channel is closed.
     */
    auto try_send(const element_type &element) -> channel_result::send;

    /**
     * @brief Non-blocking send. Does not suspend.
     *
     * @return channel_result::send::kSent on success, kFull when the channel
     *         has no free slot (mutex contention is treated as kFull, like
     *         tokio::sync::mpsc), and kClosed if the channel is closed.
     */
    auto try_send(element_type &&element) -> channel_result::send;

    /**
     * @brief Receives an element from the channel, suspending until an element
     *        is available or the channel is closed and drained.
     *
     * @return The element, or channel_result::recv::kClosed if the channel is
     *         closed and no buffered elements remain.
     */
    [[nodiscard]] auto recv() -> silicon::scheduler::task<expected<element_type, channel_result::recv>>;

    /**
     * @brief Non-blocking receive. Does not suspend.
     *
     * @return The element, channel_result::recv::kEmpty when the channel has
     *         no element (mutex contention is treated as kEmpty, like
     *         tokio::sync::mpsc), or kClosed if the channel is closed and
     *         drained.
     */
    [[nodiscard]] auto try_recv() -> expected<element_type, channel_result::recv>;

    /**
     * @brief Closes the channel. Idempotent. Buffered elements are preserved
     *        and can still be received (drain semantics), then recv() reports
     *        kClosed. All suspended waiters are woken up with the closed result.
     */
    auto close() -> silicon::scheduler::task<void>;

    /**
     * @return true if close() has been called.
     */
    [[nodiscard]] auto closed() const -> bool;

    /**
     * @return The maximum number of buffered elements (0 = unbuffered).
     */
    [[nodiscard]] auto capacity() const -> size_t;

    /**
     * @return The number of elements currently buffered.
     */
    [[nodiscard]] auto size() const -> size_t;

    /**
     * @return true if the channel currently buffers zero elements.
     */
    [[nodiscard]] auto empty() const -> bool;

    /**
     * @return true if the channel buffer has no free slot (always true for an
     *         unbuffered channel with capacity 0).
     */
    [[nodiscard]] auto full() const -> bool;

  private:
    friend send_operation;
    friend recv_operation;

    auto do_try_send(element_type element) -> channel_result::send;
    auto try_resume_senders() -> silicon::scheduler::task<void>;
    auto try_resume_receivers() -> silicon::scheduler::task<void>;

    struct impl {
      public:
        explicit impl(size_t capacity);

        auto store(element_type &&element) -> void;
        auto take() -> std::optional<element_type>;
        auto append_send_waiter(send_operation *op) -> void;
        auto pop_send_waiter() -> send_operation *;
        auto append_recv_waiter(recv_operation *op) -> void;
        auto pop_recv_waiter() -> recv_operation *;

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

} // namespace silicon::coroutine

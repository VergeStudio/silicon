module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <array>
#include <atomic>
#include <coroutine>
#include <memory>
#include <optional>
#include <utility>

export module silicon.coroutine:ring_buffer;

import silicon.scheduler;
import :mutex;
import silicon.scheduler.task;
export namespace silicon::coroutine {
namespace ring_buffer_result {
enum class produce {
    kProduced,
    kNotified,
    kStopped
};

enum class consume {
    kNotified,
    kStopped
};
} // namespace ring_buffer_result

/**
 * @tparam element The type of element the ring buffer will store.  Note that this type should be
 *         cheap to move if possible as it is moved into and out of the buffer upon produce and
 *         consume operations.
 * @tparam num_elements The maximum number of elements the ring buffer can store, must be >= 1.
 *
 * @note 实现（成员函数体）位于 coroutine/src/ring_buffer.cpp（主模块实现单元），
 *       本接口分区仅保留声明与嵌套类型布局，以降低编辑实现时的重编波及面。
 */
template<typename element, size_t num_elements>
class ring_buffer {
  private:
    enum class running_state_t {
        /// @brief The ring buffer is still running.
        kRunning,
        /// @brief The ring buffer is draining all elements, produce is no longer allowed.
        kDraining,
        /// @brief The ring buffer is fully shutdown, all produce and consume tasks will be woken up with result::stopped.
        kStopped,
    };

  public:
    /**
     * static_assert If `num_elements` == 0.
     */
    ring_buffer();

    ~ring_buffer();

    ring_buffer(const ring_buffer<element, num_elements> &) = delete;
    ring_buffer(ring_buffer<element, num_elements> &&) = delete;

    ring_buffer<element, num_elements> & operator=(const ring_buffer<element, num_elements> &) noexcept = delete;
    ring_buffer<element, num_elements> & operator=(ring_buffer<element, num_elements> &&) noexcept = delete;

    struct produce_operation {
        produce_operation(ring_buffer<element, num_elements> &rb, element);

        bool await_ready() noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;

        /**
         * @return produce_result
         */
        auto await_resume() -> ring_buffer_result::produce;

        /// If the operation needs to suspend, the coroutine to resume when the element can be produced.
        std::coroutine_handle<> m_awaiting_coroutine;
        /// The result that should be returned when this coroutine resumes.
        ring_buffer_result::produce m_result{ring_buffer_result::produce::kProduced};
        /// Linked list of produce operations that are awaiting to produce their element.
        produce_operation *m_next{nullptr};

      private:
        template<typename element_subtype, size_t num_elements_subtype>
        friend class ring_buffer;

        /// The ring buffer the element is being produced into.
        ring_buffer<element, num_elements> &m_rb;
        /// The element this produce operation is producing into the ring buffer.
        std::optional<element> m_e{std::nullopt};
    };

    struct consume_operation {
        explicit consume_operation(ring_buffer<element, num_elements> &rb);

        bool await_ready() noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;

        /**
         * @return The consumed element or ring_buffer_stopped if the ring buffer has been shutdown.
         */
        auto await_resume() -> expected<element, ring_buffer_result::consume>;

        /// If the operation needs to suspend, the coroutine to resume when the element can be consumed.
        std::coroutine_handle<> m_awaiting_coroutine;
        /// The unexpected result this should return on resume
        ring_buffer_result::consume m_result{ring_buffer_result::consume::kStopped};
        /// Linked list of consume operations that are awaiting to consume an element.
        consume_operation *m_next{nullptr};

      private:
        template<typename element_subtype, size_t num_elements_subtype>
        friend class ring_buffer;

        /// The ring buffer to consume an element from.
        ring_buffer<element, num_elements> &m_rb;
        /// The element this consume operation will consume.
        std::optional<element> m_e{std::nullopt};
    };

    /**
     * Produces the given element into the ring buffer.  This operation will suspend until a slot
     * in the ring buffer becomes available.
     * @param e The element to produce.
     */
    [[nodiscard]] silicon::scheduler::task<ring_buffer_result::produce> produce(element) ;

    /**
     * Consumes an element from the ring buffer.  This operation will suspend until an element in
     * the ring buffer becomes available.
     */
    [[nodiscard]] silicon::scheduler::task<expected<element, ring_buffer_result::consume>> consume() ;

    /**
     * @return The maximum number of elements the ring buffer can hold.
     */
    constexpr size_t max_size() const noexcept { return num_elements; }

    /**
     * @return The current number of elements contained in the ring buffer.
     */
    size_t size() const ;

    /**
     * @return True if the ring buffer contains zero elements.
     */
    [[nodiscard]] bool empty() const ;

    /**
     * @return True if the ring buffer has no more space.
     */
    bool full() const ;

    /**
     * @brief Wakes up all currently awaiting producers.  Their await_resume() function
     *        will return an expected produce result that producers have been notified.
     */
    silicon::scheduler::task<void> notify_producers() ;

    /**
     * @brief Wakes up all currently awaiting consumers.  Their await_resume() function
     *        will return an expected consume result that consumers have been notified.
     */
    silicon::scheduler::task<void> notify_consumers() ;

    /**
     * @brief Wakes up all currently awaiting producers and consumers.  Their await_resume() function
     *        will return an expected consume result that the ring buffer has stopped.
     */
    silicon::scheduler::task<void> shutdown() ;

    template<silicon::coroutine::concepts::executor executor_type>
    [[nodiscard]] silicon::scheduler::task<void> shutdown_drain(std::unique_ptr<executor_type> &e) ;

    /**
     * Returns true if shutdown() or shutdown_drain() have been called on this silicon::coroutine::ring_buffer.
     * @return True if the silicon::coroutine::ring_buffer has been shutdown.
     */
    [[nodiscard]] bool is_shutdown() const ;

  private:
    friend produce_operation;
    friend consume_operation;

    struct impl {
      public:
        silicon::coroutine::mutex m_mutex{};

        std::array<std::optional<element>, num_elements> m_elements{};
        /// The current front pointer to an open slot if not full.
        std::atomic<size_t> m_front{0};
        /// The current back pointer to the oldest item in the buffer if not empty.
        std::atomic<size_t> m_back{0};
        /// The number of items in the ring buffer.
        std::atomic<size_t> m_used{0};

        /// The LIFO list of produce waiters.
        std::atomic<produce_operation *> m_produce_waiters{nullptr};
        /// The LIFO list of consume watier.
        std::atomic<consume_operation *> m_consume_waiters{nullptr};

        std::atomic<running_state_t> m_running_state{running_state_t::kRunning};
    };

    std::unique_ptr<impl> m_p;

    silicon::scheduler::task<void> try_resume_producers() ;
    silicon::scheduler::task<void> try_resume_consumers() ;
};

} // namespace silicon::coroutine

module;

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
}

template<typename element, size_t num_elements>
class ring_buffer {
  private:
    enum class running_state_t {

        kRunning,

        kDraining,

        kStopped,
    };

  public:

    ring_buffer();

    ~ring_buffer();

    ring_buffer(const ring_buffer<element, num_elements> &) = delete;
    ring_buffer(ring_buffer<element, num_elements> &&) = delete;

    ring_buffer<element, num_elements> & operator=(const ring_buffer<element, num_elements> &) noexcept = delete;
    ring_buffer<element, num_elements> & operator=(ring_buffer<element, num_elements> &&) noexcept = delete;

    struct produce_operation {
        produce_operation(ring_buffer<element, num_elements> &, element);

        bool await_ready() noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;

        auto await_resume() -> ring_buffer_result::produce;

        std::coroutine_handle<> m_awaiting_coroutine;

        ring_buffer_result::produce m_result{ring_buffer_result::produce::kProduced};

        produce_operation *m_next{nullptr};

      private:
        template<typename element_subtype, size_t num_elements_subtype>
        friend class ring_buffer;

        ring_buffer<element, num_elements> &m_rb;

        std::optional<element> m_e{std::nullopt};
    };

    struct consume_operation {
        explicit consume_operation(ring_buffer<element, num_elements> &);

        bool await_ready() noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;

        auto await_resume() -> silicon::scheduler::expected<element, ring_buffer_result::consume>;

        std::coroutine_handle<> m_awaiting_coroutine;

        ring_buffer_result::consume m_result{ring_buffer_result::consume::kStopped};

        consume_operation *m_next{nullptr};

      private:
        template<typename element_subtype, size_t num_elements_subtype>
        friend class ring_buffer;

        ring_buffer<element, num_elements> &m_rb;

        std::optional<element> m_e{std::nullopt};
    };

    [[nodiscard]] silicon::scheduler::task<ring_buffer_result::produce> produce(element) ;

    [[nodiscard]] silicon::scheduler::task<silicon::scheduler::expected<element, ring_buffer_result::consume>> consume() ;

    constexpr size_t max_size() const noexcept { return num_elements; }

    size_t size() const ;

    [[nodiscard]] bool empty() const ;

    bool full() const ;

    silicon::scheduler::task<void> notify_producers() ;

    silicon::scheduler::task<void> notify_consumers() ;

    silicon::scheduler::task<void> shutdown() ;

    template<silicon::scheduler::concepts::executor executor_type>
    [[nodiscard]] silicon::scheduler::task<void> shutdown_drain(std::unique_ptr<executor_type> &) ;

    [[nodiscard]] bool is_shutdown() const ;

  private:
    friend produce_operation;
    friend consume_operation;

    struct impl {
      public:
        silicon::coroutine::mutex m_mutex{};

        std::array<std::optional<element>, num_elements> m_elements{};

        std::atomic<size_t> m_front{0};

        std::atomic<size_t> m_back{0};

        std::atomic<size_t> m_used{0};

        std::atomic<produce_operation *> m_produce_waiters{nullptr};

        std::atomic<consume_operation *> m_consume_waiters{nullptr};

        std::atomic<running_state_t> m_running_state{running_state_t::kRunning};
    };

    std::unique_ptr<impl> m_p;

    silicon::scheduler::task<void> try_resume_producers() ;
    silicon::scheduler::task<void> try_resume_consumers() ;
};

}

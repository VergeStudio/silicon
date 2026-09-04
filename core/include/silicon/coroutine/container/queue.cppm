module;

#include <optional>
#include <memory>
#include <utility>
#include <coroutine>
#include <atomic>
#include <queue>

export module silicon.coroutine:queue;

import silicon.scheduler;
import :mutex;
import silicon.scheduler.task;
export namespace silicon::coroutine {

enum class queue_produce_result {

    kProduced,

    kStopped
};

enum class queue_consume_result {

    kStopped,

    kTryLockFailure,

    kEmpty,
};

template<typename element_type>
class queue {
  private:
    enum class running_state_t {
        kRunning,
        kDraining,
        kStopped,
    };

  public:
    struct awaiter {
        explicit awaiter(queue<element_type> &) noexcept;

        bool await_ready() noexcept ;
        bool await_suspend(std::coroutine_handle<>) noexcept ;
        [[nodiscard]] silicon::scheduler::expected<element_type, queue_consume_result> await_resume() noexcept ;

        std::optional<element_type> m_element{std::nullopt};
        queue &m_queue;
        std::coroutine_handle<> m_awaiting_coroutine{nullptr};
        awaiter *m_next{nullptr};
    };

    queue();
    ~queue();

    queue(const queue &) = delete;
    queue(queue &&other) = delete;

    queue & operator=(const queue &) = delete;
    queue & operator=(queue &&other) = delete;

    bool empty() const ;

    std::size_t size() const ;

    silicon::scheduler::task<queue_produce_result> push(const element_type &) ;

    silicon::scheduler::task<queue_produce_result> push(element_type &&element) ;

    template<typename... args_type>
    silicon::scheduler::task<queue_produce_result> emplace(args_type &&...) ;

    [[nodiscard]] silicon::scheduler::task<silicon::scheduler::expected<element_type, queue_consume_result>> pop() ;

    [[nodiscard]] silicon::scheduler::expected<element_type, queue_consume_result> try_pop() ;

    silicon::scheduler::task<void> shutdown() ;

    template<silicon::scheduler::concepts::executor executor_type>
    silicon::scheduler::task<void> shutdown_drain(std::unique_ptr<executor_type> &) ;

    [[nodiscard]] bool is_shutdown() const ;

  private:
    friend awaiter;

    struct impl {
      public:

        awaiter *m_waiters{nullptr};

        silicon::coroutine::mutex m_mutex{};

        std::queue<element_type> m_elements{};

        std::atomic<running_state_t> m_running_state{running_state_t::kRunning};
    };

    std::unique_ptr<impl> m_p;
};

}

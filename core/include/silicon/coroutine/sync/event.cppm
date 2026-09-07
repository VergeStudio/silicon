module;

#include <atomic>
#include <coroutine>
#include <memory>
#include <vector>

#include "silicon/common.h"

export module silicon.coroutine:event;

import silicon.scheduler;

export namespace silicon::coroutine {
enum class resume_order_policy {

    kLifo,

    kFifo
};

class SILICON_CORE_API event {
  public:

    struct SILICON_CORE_API awaiter {

        awaiter(const event &e) noexcept: m_event(e) {}

        bool await_ready() const noexcept { return m_event.is_set(); }

        bool await_suspend(std::coroutine_handle<>) noexcept ;

        auto await_resume() noexcept {}

        awaiter *m_next{nullptr};

        std::coroutine_handle<> m_awaiting_coroutine;

        const event &m_event;
    };

    explicit event(bool = false) noexcept;
    ~event();

    event(const event &) = delete;
    event(event &&) = delete;
    event & operator=(const event &) = delete;
    event & operator=(event &&) = delete;

    bool is_set() const noexcept ;

    void set(resume_order_policy = resume_order_policy::kLifo) noexcept ;

    template<silicon::scheduler::concepts::executor executor_type>
    void set(std::unique_ptr<executor_type> &e, resume_order_policy policy = resume_order_policy::kLifo) noexcept {
        void *old_value = exchange_set_state();
        if(old_value != this) {

            if(policy == resume_order_policy::kFifo) {
                old_value = reverse(static_cast<awaiter *>(old_value));
            }

            auto *waiters = static_cast<awaiter *>(old_value);
            while(waiters != nullptr) {
                auto *next = waiters->m_next;
                e->resume(waiters->m_awaiting_coroutine);
                waiters = next;
            }
        }
    }

    auto operator co_await() const noexcept -> awaiter { return awaiter(*this); }

    void reset() noexcept ;

  private:

    friend struct awaiter;

    struct impl;

    std::unique_ptr<impl> m_p;

    auto reverse(awaiter *) -> awaiter *;

    void * exchange_set_state() noexcept ;
};

}

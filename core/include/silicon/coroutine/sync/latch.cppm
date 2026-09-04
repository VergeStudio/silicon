module;

#include <memory>

#include <atomic>

#include <silicon/common.h>
export module silicon.coroutine:latch;

import :event;
import silicon.scheduler;

export namespace silicon::coroutine {

class CORE_API latch {
  public:

    latch(std::int64_t) noexcept;
    ~latch();

    latch(const latch &) = delete;
    latch(latch &&) = delete;
    latch & operator=(const latch &) = delete;
    latch & operator=(latch &&) = delete;

    bool is_ready() const noexcept ;

    std::size_t remaining() const noexcept ;

    void count_down(std::int64_t = 1) noexcept ;

    template<silicon::scheduler::concepts::executor executor_type>
    void count_down(std::unique_ptr<executor_type> &executor, std::int64_t n = 1) noexcept {
        if(decrement(n)) {
            internal_event().set(executor);
        }
    }

    auto operator co_await() const noexcept -> event::awaiter;

  private:

    struct impl;

    std::unique_ptr<impl> m_p;

    bool decrement(std::int64_t) noexcept ;

    event & internal_event() noexcept ;
};

}

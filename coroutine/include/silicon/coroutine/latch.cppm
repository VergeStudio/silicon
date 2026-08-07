module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <memory>



#include <atomic>


export module silicon.coroutine:latch;

import :event;
import silicon.scheduler;

export namespace silicon::coroutine {
/**
 * The latch is thread safe counter to wait for 1 or more other tasks to complete, they signal their
 * completion by calling `count_down()` on the latch and upon the latch counter reaching zero the
 * coroutine `co_await`ing the latch then resumes execution.
 *
 * This is useful for spawning many worker tasks to complete either a computationally complex task
 * across a thread pool of workers, or waiting for many asynchronous results like http requests
 * to complete.
 */
class latch {
  public:
    /**
     * Creates a latch with the given count of tasks to wait to complete.
     * @param count The number of tasks to wait to complete, if this is zero or negative then the
     *              latch starts 'completed' immediately and execution is resumed with no suspension.
     */
    latch(std::int64_t count) noexcept;
    ~latch();

    latch(const latch &) = delete;
    latch(latch &&) = delete;
    auto operator=(const latch &) -> latch & = delete;
    auto operator=(latch &&) -> latch & = delete;

    /**
     * @return True if the latch has been counted down to zero.
     */
    auto is_ready() const noexcept -> bool;

    /**
     * @return The number of tasks this latch is still waiting to complete.
     */
    auto remaining() const noexcept -> std::size_t;

    /**
     * If the latch counter goes to zero then the task awaiting the latch is resumed.
     * @param n The number of tasks to complete towards the latch, defaults to 1.
     */
    auto count_down(std::int64_t n = 1) noexcept -> void;

    /**
     * If the latch counter goes to zero then the task awaiting the latch is resumed on the given
     * thread pool.
     * @param tp The thread pool to schedule the task that is waiting on the latch on.
     * @param n The number of tasks to complete towards the latch, defaults to 1.
     */
    template<concepts::executor executor_type>
    auto count_down(std::unique_ptr<executor_type> &executor, std::int64_t n = 1) noexcept -> void {
        if(decrement(n)) {
            internal_event().set(executor);
        }
    }

    auto operator co_await() const noexcept -> event::awaiter;

  private:
    /// PIMPL：Impl 仅前置声明，定义置于 src/latch.cpp。
    struct Impl;
    /// Hidden implementation state.
    std::unique_ptr<Impl> m_p;

    /**
     * 非模板钩子：递减计数，返回是否刚好归零（需要触发内部 event）。
     * 供接口单元中的 `count_down(executor)` 模板重载使用。
     */
    auto decrement(std::int64_t n) noexcept -> bool;
    /// 非模板钩子：暴露内部 event 引用，供模板重载在 executor 上恢复等待者。
    auto internal_event() noexcept -> event &;
};

} // namespace silicon::coroutine

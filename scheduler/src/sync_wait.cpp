module;

// 实现单元全局片段：补齐 std 头，供 sync_wait_event::impl 定义与成员函数使用。
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <coroutine>
#include <map>
#include <optional>

module silicon.scheduler;
#include "poll_info_impl.hpp"

namespace silicon::coroutine {

class sync_wait_event::impl {
  public:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_set{false};
};

sync_wait_event::sync_wait_event(bool initially_set): m_p(std::make_unique<impl>()) {
    m_p->m_set = initially_set;
}

sync_wait_event::~sync_wait_event() = default;

auto sync_wait_event::set() noexcept -> void {
    // issue-270 100~ task's on a thread_pool within sync_wait(when_all(tasks)) can cause a deadlock/hang if using
    // release/acquire or even seq_cst.
    {
        std::unique_lock<std::mutex> lk{m_p->m_mutex};
        m_p->m_set.exchange(true, std::memory_order::seq_cst);
        m_p->m_cv.notify_all();
    }
}

auto sync_wait_event::reset() noexcept -> void {
    m_p->m_set.exchange(false, std::memory_order::seq_cst);
}

auto sync_wait_event::wait() noexcept -> void {
    std::unique_lock<std::mutex> lk{m_p->m_mutex};
    m_p->m_cv.wait(lk, [this] { return m_p->m_set.load(std::memory_order::seq_cst); });
}

} // namespace silicon::coroutine

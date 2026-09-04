module;

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <coroutine>
#include <map>
#include <optional>

module silicon.scheduler;

import :poll_info_impl;

namespace silicon::scheduler {

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

void sync_wait_event::set() noexcept {

    {
        std::unique_lock<std::mutex> lk{m_p->m_mutex};
        m_p->m_set.exchange(true, std::memory_order::seq_cst);
        m_p->m_cv.notify_all();
    }
}

void sync_wait_event::reset() noexcept {
    m_p->m_set.exchange(false, std::memory_order::seq_cst);
}

void sync_wait_event::wait() noexcept {
    std::unique_lock<std::mutex> lk{m_p->m_mutex};
    m_p->m_cv.wait(lk, [this] { return m_p->m_set.load(std::memory_order::seq_cst); });
}

}

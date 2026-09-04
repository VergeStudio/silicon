module;

#include <atomic>
#include <coroutine>
#include <cstdint>

module silicon.scheduler.task;

namespace silicon::scheduler {

task_event::task_event(bool initially_set) noexcept
    : m_state(initially_set ? static_cast<void *>(this) : nullptr) {}

bool task_event::is_set() const noexcept {
    return m_state.load(std::memory_order::acquire) == this;
}

bool task_event::awaiter::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
    m_awaiting_coroutine = awaiting_coroutine;
    void *expected = nullptr;
    m_next = nullptr;
    if(m_event.m_state.compare_exchange_strong(expected, this, std::memory_order::acq_rel, std::memory_order::relaxed)) {
        return true;
    }
    if(expected == &m_event) {
        return false;
    }
    m_next = static_cast<awaiter *>(expected);
    while(true) {
        expected = m_next;
        if(m_event.m_state.compare_exchange_strong(expected, this, std::memory_order::acq_rel, std::memory_order::acquire)) {
            return true;
        }
        if(expected == &m_event) {
            return false;
        }
        m_next = static_cast<awaiter *>(expected);
    }
}

void task_event::set() noexcept {
    void *old_value = m_state.exchange(this, std::memory_order::acq_rel);
    if(old_value != this && old_value != nullptr) {
        auto *waiters = static_cast<awaiter *>(old_value);
        while(waiters != nullptr) {
            auto *next = waiters->m_next;
            waiters->m_awaiting_coroutine.resume();
            waiters = next;
        }
    }
}

void task_event::reset() noexcept {
    void *expected = this;
    m_state.compare_exchange_strong(expected, nullptr, std::memory_order::acq_rel, std::memory_order::relaxed);
}

auto task_event::operator co_await() const noexcept -> awaiter {
    return awaiter(*this);
}

}

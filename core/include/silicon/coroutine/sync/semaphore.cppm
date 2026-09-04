module;

#include <atomic>
#include <coroutine>
#include <memory>
#include <string>

#include "silicon/common.h"

export module silicon.coroutine:semaphore;

import silicon.scheduler;
import :mutex;

export namespace silicon::coroutine {

enum class semaphore_acquire_result {

    kAcquired,

    kShutdown
};

extern CORE_API std::string semaphore_acquire_result_acquired;
extern CORE_API std::string semaphore_acquire_result_shutdown;
extern CORE_API std::string semaphore_acquire_result_unknown;

CORE_API auto to_string(semaphore_acquire_result) -> const std::string &;

template<std::ptrdiff_t max_value>
class semaphore;

template<std::ptrdiff_t max_value>
class acquire_operation {
  public:
    explicit acquire_operation(semaphore<max_value> &s): m_semaphore(s) {}

    [[nodiscard]] bool await_ready() const noexcept {

        if(m_semaphore.m_p->m_shutdown.load(std::memory_order::acquire) || m_semaphore.try_acquire()) {
            static_cast<void>(m_semaphore.m_p->m_mutex.unlock());
            return true;
        }

        return false;
    }

    bool await_suspend(const std::coroutine_handle<> awaiting_coroutine) noexcept {

        if(await_ready()) {
            return false;
        }

        m_awaiting_coroutine = awaiting_coroutine;
        silicon::scheduler::awaiter_list_push(m_semaphore.m_p->m_acquire_waiters, this);
        static_cast<void>(m_semaphore.m_p->m_mutex.unlock());
        return true;
    }

    [[nodiscard]] semaphore_acquire_result await_resume() const {
        if(m_semaphore.m_p->m_shutdown.load(std::memory_order::acquire)) {
            return semaphore_acquire_result::kShutdown;
        }
        return semaphore_acquire_result::kAcquired;
    }

    acquire_operation<max_value> *m_next{nullptr};
    semaphore<max_value> &m_semaphore;
    std::coroutine_handle<> m_awaiting_coroutine;
};

template<std::ptrdiff_t max_value>
class semaphore {
  public:
    explicit semaphore(const std::ptrdiff_t starting_value)
        : m_p(std::make_unique<impl>(starting_value)) {}

    ~semaphore() { silicon::scheduler::sync_wait(shutdown()); }

    semaphore(const semaphore &) = delete;
    semaphore(semaphore &&) = delete;

    semaphore & operator=(const semaphore &) noexcept = delete;
    semaphore & operator=(semaphore &&) noexcept = delete;

    [[nodiscard]] silicon::scheduler::task<semaphore_acquire_result> acquire() {
        co_await m_p->m_mutex.lock();
        co_return co_await acquire_operation<max_value>{*this};
    }

    [[nodiscard]] silicon::scheduler::task<void> release() {
        co_await m_p->m_mutex.lock();

        if(value() == max()) {
            static_cast<void>(m_p->m_mutex.unlock());
            co_return;
        }

        auto *waiter = silicon::scheduler::awaiter_list_pop(m_p->m_acquire_waiters);
        if(waiter != nullptr) {
            static_cast<void>(m_p->m_mutex.unlock());
            waiter->m_awaiting_coroutine.resume();
        } else {

            m_p->m_counter.fetch_add(1, std::memory_order::release);
            static_cast<void>(m_p->m_mutex.unlock());
        }
    }

    bool try_acquire() {
        auto expected = m_p->m_counter.load(std::memory_order::acquire);
        do {
            if(expected <= 0) {
                return false;
            }
        } while(!m_p->m_counter.compare_exchange_weak(expected, expected - 1, std::memory_order::acq_rel, std::memory_order::acquire));

        return true;
    }

    [[nodiscard]] static constexpr std::ptrdiff_t max() noexcept { return max_value; }

    [[nodiscard]] std::ptrdiff_t value() const noexcept { return m_p->m_counter.load(std::memory_order::acquire); }

    [[nodiscard]] silicon::scheduler::task<void> shutdown() noexcept {
        if(is_shutdown()) {
            co_return;
        }

        auto lock = co_await m_p->m_mutex.scoped_lock();
        bool expected{false};
        if(m_p->m_shutdown.compare_exchange_strong(expected, true, std::memory_order::release, std::memory_order::relaxed)) {
            auto *waiter = silicon::scheduler::awaiter_list_pop_all(m_p->m_acquire_waiters);
            lock.unlock();
            while(waiter != nullptr) {
                auto *next = waiter->m_next;
                waiter->m_awaiting_coroutine.resume();
                waiter = next;
            }
        }
    }

    [[nodiscard]] bool is_shutdown() const { return m_p->m_shutdown.load(std::memory_order::acquire); }

  private:
    friend class acquire_operation<max_value>;

    struct impl {
      public:
        explicit impl(const std::ptrdiff_t starting_value): m_counter(starting_value) {}

        std::atomic<std::ptrdiff_t> m_counter;

        std::atomic<acquire_operation<max_value> *> m_acquire_waiters{nullptr};

        silicon::coroutine::mutex m_mutex;

        std::atomic<bool> m_shutdown{false};
    };

    std::unique_ptr<impl> m_p;
};

}

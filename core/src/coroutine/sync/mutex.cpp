module;

#include <atomic>
#include <cstdio>
#include <memory>
#include <expected>

module silicon.coroutine;

namespace silicon::coroutine {

class mutex::impl {
  public:

    std::atomic<void *> m_state;
};

bool lock_operation_base::await_ready() const noexcept {
    return m_mutex.try_lock();
}

bool lock_operation_base::await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {
    m_awaiting_coroutine = awaiting_coroutine;
    auto &state = m_mutex.m_p->m_state;
    void *current = state.load(std::memory_order::acquire);
    const void *unlocked_value = m_mutex.unlocked_value();
    do {

        if(current == unlocked_value) {

            if(state.compare_exchange_weak(current, nullptr, std::memory_order::acq_rel, std::memory_order::acquire)) {

                m_awaiting_coroutine = nullptr;
                return false;
            }
        } else
        {

            m_next = static_cast<lock_operation_base *>(current);
            if(state.compare_exchange_weak(current, static_cast<void *>(this), std::memory_order::acq_rel, std::memory_order::acquire)) {

                return true;
            }
        }
    } while(true);
}

struct scoped_lock::impl {
  public:
    class silicon::coroutine::mutex *m_mutex{nullptr};
};

scoped_lock::scoped_lock(class silicon::coroutine::mutex &m, lock_strategy strategy)
    : m_p(std::make_unique<impl>()) {

    (void)strategy;
    m_p->m_mutex = &m;
}

scoped_lock::scoped_lock(scoped_lock &&other) noexcept: m_p(std::move(other.m_p)) {}

auto scoped_lock::operator=(scoped_lock &&other) noexcept -> scoped_lock & {
    if(std::addressof(other) != this) {
        m_p = std::move(other.m_p);
    }
    return *this;
}

auto scoped_lock::owned_mutex() const noexcept -> class silicon::coroutine::mutex * {
    return (m_p != nullptr) ? m_p->m_mutex : nullptr;
}

scoped_lock::~scoped_lock() {
    unlock();
}

void scoped_lock::unlock() {
    if(m_p != nullptr && m_p->m_mutex != nullptr) {
        std::atomic_thread_fence(std::memory_order::acq_rel);

        static_cast<void>(m_p->m_mutex->unlock());
        m_p->m_mutex = nullptr;
    }
}

mutex::mutex() noexcept: m_p(std::make_unique<impl>()) {
    m_p->m_state.store(const_cast<void *>(unlocked_value()), std::memory_order::relaxed);
}

mutex::~mutex() = default;

const void * mutex::unlocked_value() const noexcept {
    return &m_p->m_state;
}

bool mutex::try_lock() {
    void *expected = const_cast<void *>(unlocked_value());
    return m_p->m_state.compare_exchange_strong(expected, nullptr, std::memory_order::acq_rel, std::memory_order::relaxed);
}

auto mutex::unlock() -> silicon::error::result<void> {
    void *current = m_p->m_state.load(std::memory_order::acquire);
    do {

        if(current == const_cast<void *>(unlocked_value())) {
            return std::unexpected(make_error_code(coroutine_error::kAlreadyUnlocked));
        }

        if(current == nullptr) {
            if(m_p->m_state.compare_exchange_weak(
                       current,
                       const_cast<void *>(unlocked_value()),
                       std::memory_order::acq_rel,
                       std::memory_order::acquire
               )) {

                std::atomic_thread_fence(std::memory_order::acq_rel);
                return {};
            } else {

                continue;
            }
        } else {

            std::atomic<lock_operation_base *> *casted =
                    reinterpret_cast<std::atomic<lock_operation_base *> *>(&m_p->m_state);
            auto *waiter = silicon::scheduler::awaiter_list_pop<lock_operation_base>(*casted);

            std::atomic_thread_fence(std::memory_order::acq_rel);
            waiter->m_awaiting_coroutine.resume();
            return {};
        }
    } while(true);
}

}

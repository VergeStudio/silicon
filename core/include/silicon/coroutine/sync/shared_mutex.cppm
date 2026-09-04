module;


#include <atomic>
#include <memory>
#include <expected>
#include <system_error>
#include <coroutine>


export module silicon.coroutine:shared_mutex;

import silicon.scheduler;
import :mutex;
import silicon.scheduler.task;
import silicon.coroutine.error;
export namespace silicon::coroutine {
template<silicon::scheduler::concepts::executor executor_type>
class shared_mutex;


template<silicon::scheduler::concepts::executor executor_type>
struct shared_lock_operation {
    explicit shared_lock_operation(silicon::coroutine::shared_mutex<executor_type> &shared_mutex, const bool exclusive)
        : m_shared_mutex(shared_mutex),
          m_exclusive(exclusive) {}
    ~shared_lock_operation() = default;

    shared_lock_operation(const shared_lock_operation &) = delete;
    shared_lock_operation(shared_lock_operation &&) = delete;
    shared_lock_operation & operator=(const shared_lock_operation &) = delete;
    shared_lock_operation & operator=(shared_lock_operation &&) = delete;

    bool await_ready() const noexcept {


        if(m_exclusive) {
            if(m_shared_mutex.try_lock_locked()) {
                static_cast<void>(m_shared_mutex.m_p->m_mutex.unlock());
                return true;
            }
        } else if(m_shared_mutex.try_lock_shared_locked()) {
            static_cast<void>(m_shared_mutex.m_p->m_mutex.unlock());
            return true;
        }

        return false;
    }

    bool await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept {



        auto *tail_waiter = m_shared_mutex.m_p->m_tail_waiter.load(std::memory_order::acquire);

        if(tail_waiter == nullptr) {
            m_shared_mutex.m_p->m_head_waiter = this;
            m_shared_mutex.m_p->m_tail_waiter = this;
        } else {
            tail_waiter->m_next = this;
            m_shared_mutex.m_p->m_tail_waiter = this;
        }



        if(m_exclusive) {
            ++m_shared_mutex.m_p->m_exclusive_waiters;
        }

        m_awaiting_coroutine = awaiting_coroutine;
        static_cast<void>(m_shared_mutex.m_p->m_mutex.unlock());
        return true;
    }

    void await_resume() noexcept {}

  protected:
    friend class silicon::coroutine::shared_mutex<executor_type>;

    std::coroutine_handle<> m_awaiting_coroutine;
    shared_lock_operation *m_next{nullptr};
    silicon::coroutine::shared_mutex<executor_type> &m_shared_mutex;
    bool m_exclusive{false};
};



template<silicon::scheduler::concepts::executor executor_type>
class shared_mutex {
  public:
    
  private:
    explicit shared_mutex(std::unique_ptr<executor_type> &e): m_p(std::make_unique<impl>()) {
        m_p->m_executor = e.get();
    }

  public:
    
    static std::expected<std::unique_ptr<shared_mutex<executor_type>>, std::error_code> create(std::unique_ptr<executor_type> &e) {
        if(e == nullptr) {
            return std::unexpected(make_error_code(coroutine_error::kNullExecutor));
        }

        return std::unique_ptr<shared_mutex<executor_type>>(
            new shared_mutex<executor_type>(e));
    }

    ~shared_mutex() = default;

    shared_mutex(const shared_mutex &) = delete;
    shared_mutex(shared_mutex &&) = delete;
    shared_mutex & operator=(const shared_mutex &) = delete;
    shared_mutex & operator=(shared_mutex &&) = delete;

    
    [[nodiscard]] silicon::scheduler::task<void> scoped_lock_shared(silicon::scheduler::task<void> scoped_task) {
        co_await m_p->m_mutex.lock();
        co_await shared_lock_operation<executor_type>{*this, false};
        co_await scoped_task;
        co_await unlock_shared();
        co_return;
    }

    
    [[nodiscard]] silicon::scheduler::task<void> scoped_lock(silicon::scheduler::task<void> scoped_task) {
        co_await m_p->m_mutex.lock();
        co_await shared_lock_operation<executor_type>{*this, true};
        co_await scoped_task;
        co_await unlock();
        co_return;
    }

    
    [[nodiscard]] silicon::scheduler::task<void> lock_shared() {
        co_await m_p->m_mutex.lock();
        co_await shared_lock_operation<executor_type>{*this, false};
        co_return;
    }

    
    [[nodiscard]] silicon::scheduler::task<void> lock() {
        co_await m_p->m_mutex.lock();
        co_await shared_lock_operation<executor_type>{*this, true};
        co_return;
    }

    
    [[nodiscard]] bool try_lock_shared() {






        if(m_p->m_mutex.try_lock()) {
            silicon::coroutine::scoped_lock lk{m_p->m_mutex};
            return try_lock_shared_locked();
        }
        return false;
    }

    
    [[nodiscard]] bool try_lock() {

        if(m_p->m_mutex.try_lock()) {
            silicon::coroutine::scoped_lock lk{m_p->m_mutex};
            return try_lock_locked();
        }
        return false;
    }

    
    [[nodiscard]] silicon::scheduler::task<void> unlock_shared() {
        auto lk = co_await m_p->m_mutex.scoped_lock();
        auto users = m_p->m_shared_users.fetch_sub(1, std::memory_order::acq_rel);


        if(users == 1) {
            auto *head_waiter = m_p->m_head_waiter.load(std::memory_order::acquire);
            if(head_waiter != nullptr) {
                wake_waiters(lk, head_waiter);
            } else {
                m_p->m_state = state::unlocked;
            }
        }

        co_return;
    }

    
    [[nodiscard]] silicon::scheduler::task<void> unlock() {
        auto lk = co_await m_p->m_mutex.scoped_lock();
        auto *head_waiter = m_p->m_head_waiter.load(std::memory_order::acquire);
        if(head_waiter != nullptr) {
            wake_waiters(lk, head_waiter);
        } else {
            m_p->m_state = state::unlocked;
        }

        co_return;
    }

    
    [[nodiscard]] executor_type & executor() {
        return *m_p->m_executor;
    }

  private:
    friend struct shared_lock_operation<executor_type>;

    enum class state {

        unlocked,

        locked_shared,

        locked_exclusive
    };

    struct impl {
      public:

        executor_type *m_executor{nullptr};

        silicon::coroutine::mutex m_mutex;

        std::atomic<state> m_state{state::unlocked};


        std::atomic<uint64_t> m_shared_users{0};

        std::atomic<uint64_t> m_exclusive_waiters{0};

        std::atomic<shared_lock_operation<executor_type> *> m_head_waiter{nullptr};
        std::atomic<shared_lock_operation<executor_type> *> m_tail_waiter{nullptr};
    };

    std::unique_ptr<impl> m_p;

    bool try_lock_shared_locked() {
        if(m_p->m_state == state::unlocked) {

            m_p->m_state = state::locked_shared;
            ++m_p->m_shared_users;
            return true;
        } else if(m_p->m_state == state::locked_shared && m_p->m_exclusive_waiters == 0) {


            ++m_p->m_shared_users;
            return true;
        }






        return false;
    }

    bool try_lock_locked() {
        if(m_p->m_state == state::unlocked) {
            m_p->m_state = state::locked_exclusive;
            return true;
        }
        return false;
    }

    void wake_waiters(silicon::coroutine::scoped_lock &lk, shared_lock_operation<executor_type> *head_waiter) {

        if(head_waiter->m_exclusive) {

            m_p->m_state.store(state::locked_exclusive, std::memory_order::release);
            if(head_waiter->m_next == nullptr) {

                m_p->m_head_waiter.store(nullptr, std::memory_order::release);
                m_p->m_tail_waiter.store(nullptr, std::memory_order::release);
            } else {

                m_p->m_head_waiter.store(head_waiter->m_next, std::memory_order::release);
            }

            m_p->m_exclusive_waiters.fetch_sub(1, std::memory_order::release);


            lk.unlock();
            head_waiter->m_awaiting_coroutine.resume();
        } else {


            m_p->m_state.store(state::locked_shared, std::memory_order::release);
            while(true) {
                auto *to_resume = m_p->m_head_waiter.load(std::memory_order::acquire);
                if(to_resume == nullptr || to_resume->m_exclusive) {
                    break;
                }

                if(to_resume->m_next == nullptr) {
                    m_p->m_head_waiter.store(nullptr, std::memory_order::release);
                    m_p->m_tail_waiter.store(nullptr, std::memory_order::release);
                } else {
                    m_p->m_head_waiter.store(to_resume->m_next, std::memory_order::release);
                }

                m_p->m_shared_users.fetch_add(1, std::memory_order::release);

                m_p->m_executor->resume(to_resume->m_awaiting_coroutine);
            }




            lk.unlock();
        }
    }
};

}

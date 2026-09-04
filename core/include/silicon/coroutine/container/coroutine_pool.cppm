module;


#include <atomic>
#include <chrono>
#include <coroutine>
#include <cstddef>
#include <exception>
#include <functional>
#include <memory>
#include <thread>
#include <utility>
#include <expected>
#include <system_error>

export module silicon.coroutine:coroutine_pool;

import silicon.scheduler;
import silicon.scheduler.task;
import silicon.coroutine.error;
import :channel;
import :event;
import :mutex;

export namespace silicon::coroutine {


template<silicon::scheduler::concepts::executor Executor>
class coroutine_pool {
  private:
    explicit coroutine_pool(std::shared_ptr<Executor> executor, std::size_t pool_size)
        : m_p(std::make_unique<impl>(pool_size)) {
        m_p->m_executor = std::move(executor);

        for(std::size_t i = 0; i < pool_size; ++i) {
            m_p->m_workers_active.fetch_add(1, std::memory_order::release);
            (void)m_p->m_executor->spawn_detached(worker());
        }
    }

  public:
    
    static std::expected<std::unique_ptr<coroutine_pool<Executor>>, std::error_code> create(std::shared_ptr<Executor> executor, std::size_t pool_size) {
        if(executor == nullptr) {
            return std::unexpected(make_error_code(coroutine_error::kNullExecutor));
        }
        if(pool_size == 0) {
            return std::unexpected(make_error_code(coroutine_error::kInvalidPoolSize));
        }

        return std::unique_ptr<coroutine_pool<Executor>>(
            new coroutine_pool<Executor>(std::move(executor), pool_size));
    }

    coroutine_pool(const coroutine_pool&)                    = delete;
    coroutine_pool(coroutine_pool&&)                         = delete;
    coroutine_pool& operator=(const coroutine_pool&) = delete;
    coroutine_pool& operator=(coroutine_pool&&) = delete;

    ~coroutine_pool() {
        shutdown();



        while(!empty() || m_p->m_workers_active.load(std::memory_order::acquire) > 0
              || !m_p->m_close_done.load(std::memory_order::acquire)) {
            std::this_thread::sleep_for(std::chrono::milliseconds{10});
        }
    }

    
    bool dispatch(silicon::scheduler::task<void>&& work) {
        return spawn_detached(std::move(work));
    }

    
    bool spawn_detached(silicon::scheduler::task<void>&& work) {
        if(m_p->m_stopped.load(std::memory_order::acquire)) {
            return false;
        }


        m_p->m_inflight.fetch_add(1, std::memory_order::relaxed);
        m_p->m_pending_sends.fetch_add(1, std::memory_order::relaxed);
        auto ok = m_p->m_executor->spawn_detached(sender(std::move(work)));
        if(!ok) {

            m_p->m_inflight.fetch_sub(1, std::memory_order::release);
            m_p->m_pending_sends.fetch_sub(1, std::memory_order::release);
        }
        return ok;
    }

    
    silicon::scheduler::task<void> spawn_joinable(silicon::scheduler::task<void>&& work) {
        auto e = std::make_shared<silicon::coroutine::event>();
        if(!spawn_detached(make_wrapper(this, e, std::move(work)))) {
            e->set();
        }





        return make_join_task(e);
    }



    static silicon::scheduler::task<void> make_join_task(std::shared_ptr<silicon::coroutine::event> e) {
        co_await *e;
    }



    static silicon::scheduler::task<void> make_wrapper(coroutine_pool *self, std::shared_ptr<silicon::coroutine::event> e,
                             silicon::scheduler::task<void> w) {
        try {
            co_await std::move(w);
        } catch(...) {
            self->capture_error();
        }
        e->set();
    }

    
    [[nodiscard]] std::size_t size() const {
        return m_p->m_inflight.load(std::memory_order::acquire);
    }

    
    [[nodiscard]] bool empty() const { return size() == 0; }

    
    silicon::scheduler::task<void> join() {
        while(!empty()) {
            co_await m_p->m_executor->yield();
        }
    }

    
    void shutdown() {
        if(m_p->m_stopped.exchange(true, std::memory_order::acq_rel)) {
            return;
        }
        (void)m_p->m_executor->spawn_detached(async_close());
    }


    auto schedule() { return m_p->m_executor->schedule(); }

    auto yield() { return m_p->m_executor->yield(); }

    bool resume(std::coroutine_handle<> handle) { return m_p->m_executor->resume(handle); }

    
    [[nodiscard]] std::exception_ptr last_error() const { return m_p->m_last_error; }

  private:
    void capture_error() {
        if(!m_p->m_last_error) {
            m_p->m_last_error = std::current_exception();
        }
    }


    silicon::scheduler::task<void> worker() {
        while(true) {
            auto got = co_await m_p->m_channel.recv();
            if(!got.has_value()) {
                break;
            }
            auto work = std::move(got).value();
            try {
                co_await std::move(work);
            } catch(...) {
                capture_error();
            }

            m_p->m_inflight.fetch_sub(1, std::memory_order::release);
        }

        m_p->m_workers_active.fetch_sub(1, std::memory_order::release);
    }


    silicon::scheduler::task<void> sender(silicon::scheduler::task<void> work) {
        auto result = co_await m_p->m_channel.send(std::move(work));
        if(result == channel_result::send::kClosed) {

            m_p->m_inflight.fetch_sub(1, std::memory_order::release);
        }

        m_p->m_pending_sends.fetch_sub(1, std::memory_order::release);
    }



    silicon::scheduler::task<void> async_close() {
        while(m_p->m_pending_sends.load(std::memory_order::acquire) > 0) {
            co_await m_p->m_executor->yield();
        }
        co_await m_p->m_channel.close();
        m_p->m_close_done.store(true, std::memory_order::release);
    }

    struct impl {
      public:
        explicit impl(std::size_t capacity)
            : m_channel{capacity} {}

        std::shared_ptr<Executor> m_executor{nullptr};
        channel<silicon::scheduler::task<void>> m_channel;
        std::atomic<std::size_t> m_inflight{0};
        std::atomic<std::size_t> m_pending_sends{0};
        std::atomic<std::size_t> m_workers_active{0};
        std::atomic<bool> m_close_done{false};
        std::atomic<bool> m_stopped{false};
        std::exception_ptr m_last_error{nullptr};
    };

    std::unique_ptr<impl> m_p;
};

}

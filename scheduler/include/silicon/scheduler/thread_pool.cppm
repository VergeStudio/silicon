module;

#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <system_error>
#include <thread>
#include <vector>

export module silicon.scheduler:thread_pool;

import silicon.scheduler.task;

import :facade;

export namespace silicon::scheduler {

/// @brief 基于工作窃取的 CPU 线程池调度器。
///
/// 使用 pimpl：所有数据成员位于 thread_pool::impl，定义在模块实现单元
/// （thread_pool.cpp）。模板便捷重载（schedule<return_type>、
/// resume<range_type>）在此内联定义。
class thread_pool final {
    struct private_constructor {
        explicit private_constructor() = default;
    };

  public:
    class schedule_operation {
        friend class thread_pool;
        explicit schedule_operation(thread_pool &tp) noexcept;

      public:
        bool await_ready() noexcept { return false; }
        void await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept ;
        void await_resume() noexcept {}

      private:
        thread_pool &m_thread_pool;
    };

    struct options {
        uint32_t thread_count = std::thread::hardware_concurrency();
        std::function<void(std::size_t)> on_thread_start_functor = nullptr;
        std::function<void(std::size_t)> on_thread_stop_functor = nullptr;
    };

    explicit thread_pool(options &&opts, private_constructor);

    static auto create(
            options opts = options{
                    .thread_count = std::thread::hardware_concurrency(),
                    .on_thread_start_functor = nullptr,
                    .on_thread_stop_functor = nullptr
            }
    ) -> std::expected<std::unique_ptr<thread_pool>, std::error_code>;

    thread_pool(const thread_pool &) = delete;
    thread_pool(thread_pool &&) = delete;
    thread_pool & operator=(const thread_pool &) = delete;
    thread_pool & operator=(thread_pool &&) = delete;

    ~thread_pool();

    /// @brief 线程池中的线程数（thread_pool 独有）。
    [[nodiscard]] std::size_t thread_count() const noexcept ;

    [[nodiscard]] schedule_operation schedule() ;

    bool spawn_detached(silicon::scheduler::task<void> &&task) noexcept ;
    silicon::scheduler::task<void> spawn_joinable(silicon::scheduler::task<void> &&task) noexcept ;

    template<typename return_type>
    [[nodiscard]] silicon::scheduler::task<return_type> schedule(silicon::scheduler::task<return_type> task) {
        co_await schedule();
        co_return co_await task;
    }

    bool resume(std::coroutine_handle<> handle) noexcept ;

    template<typename range_type>
        requires requires(const range_type &r) {
            std::size(r);
            std::begin(r);
            std::end(r);
        }
    std::size_t resume(const range_type &handles) noexcept {
        std::vector<std::coroutine_handle<>> vec(std::begin(handles), std::end(handles));
        return resume_range_impl(vec);
    }

    [[nodiscard]] schedule_operation yield() { return schedule(); }
    void shutdown() noexcept ;

    [[nodiscard]] bool is_shutdown() const ;
    std::size_t size() const noexcept ;
    bool empty() const noexcept { return size() == 0; }

    /// @brief 队列中等待的任务数（thread_pool 独有）。
    std::size_t queue_size() const noexcept ;
    /// @brief 任务队列是否为空（thread_pool 独有）。
    [[nodiscard]] bool queue_empty() const noexcept { return queue_size() == 0; }

  private:
    struct impl;
    std::unique_ptr<impl> m_impl;

    std::size_t resume_range_impl(std::vector<std::coroutine_handle<>> &handles) noexcept ;
};

} // namespace silicon::scheduler

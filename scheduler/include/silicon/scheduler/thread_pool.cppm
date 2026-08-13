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

import :ischeduler;

export namespace silicon::scheduler {

/// @brief 基于工作窃取的 CPU 线程池调度器。
///
/// 使用 pimpl：所有数据成员位于 thread_pool::Impl，定义在模块实现单元
/// （thread_pool.cpp）。模板便捷重载（schedule<return_type>、
/// resume<range_type>）在此内联定义。
class thread_pool final: public i_scheduler {
    struct private_constructor {
        explicit private_constructor() = default;
    };

  public:
    class schedule_operation {
        friend class thread_pool;
        explicit schedule_operation(thread_pool &tp) noexcept;

      public:
        auto await_ready() noexcept -> bool { return false; }
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> void;
        auto await_resume() noexcept -> void {}

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
    auto operator=(const thread_pool &) -> thread_pool & = delete;
    auto operator=(thread_pool &&) -> thread_pool & = delete;

    ~thread_pool() override;

    /// @brief 线程池中的线程数（thread_pool 独有）。
    [[nodiscard]] auto thread_count() const noexcept -> std::size_t;

    [[nodiscard]] auto schedule() -> schedule_operation;

    auto spawn_detached(silicon::scheduler::task<void> &&task) noexcept -> bool override;
    auto spawn_joinable(silicon::scheduler::task<void> &&task) noexcept
            -> silicon::scheduler::task<void> override;

    template<typename return_type>
    [[nodiscard]] auto schedule(silicon::scheduler::task<return_type> task)
            -> silicon::scheduler::task<return_type> {
        co_await schedule();
        co_return co_await task;
    }

    auto resume(std::coroutine_handle<> handle) noexcept -> bool override;

    template<typename range_type>
        requires requires(const range_type &r) {
            std::size(r);
            std::begin(r);
            std::end(r);
        }
    auto resume(const range_type &handles) noexcept -> std::size_t {
        std::vector<std::coroutine_handle<>> vec(std::begin(handles), std::end(handles));
        return resume_range_impl(vec);
    }

    [[nodiscard]] auto yield() -> schedule_operation { return schedule(); }
    auto shutdown() noexcept -> void override;

    [[nodiscard]] auto is_shutdown() const -> bool override;
    auto size() const noexcept -> std::size_t override;
    auto empty() const noexcept -> bool override { return size() == 0; }

    /// @brief 队列中等待的任务数（thread_pool 独有）。
    auto queue_size() const noexcept -> std::size_t;
    /// @brief 任务队列是否为空（thread_pool 独有）。
    [[nodiscard]] auto queue_empty() const noexcept -> bool { return queue_size() == 0; }

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    auto resume_range_impl(std::vector<std::coroutine_handle<>> &handles) noexcept -> std::size_t;
};

} // namespace silicon::scheduler

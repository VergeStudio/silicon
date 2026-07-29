module;

#include <coroutine>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

export module silicon.thread;

import silicon.task;

export import :config;

export namespace silicon::thread {

/// @brief Abstract interface for a thread pool.
///
/// All non-template public methods of pool are declared here as pure virtual.
/// Template methods (schedule<return_type>, resume<range_type>) remain in the
/// concrete pool class — they cannot be virtual but are implemented in terms
/// of the virtual methods exposed here.
class IPool {
  public:
    IPool() = default;
    IPool(const IPool &) = delete;
    IPool(IPool &&) = delete;
    auto operator=(const IPool &) -> IPool & = delete;
    auto operator=(IPool &&) -> IPool & = delete;

    virtual ~IPool() = default;

    /// @brief Returns the number of threads in the pool.
    [[nodiscard]] virtual auto thread_count() const noexcept -> std::size_t = 0;

    /// @brief Spawns a void task onto the pool (detached).
    virtual auto spawn_detached(silicon::task::task<void> &&task) noexcept -> bool = 0;

    /// @brief Spawns a task onto the pool, returning a joinable task.
    virtual auto spawn_joinable(silicon::task::task<void> &&task) noexcept
            -> silicon::task::task<void> = 0;

    /// @brief Resumes a single coroutine handle on this pool.
    virtual auto resume(std::coroutine_handle<> handle) noexcept -> bool = 0;

    /// @brief Starts shutdown; blocks until all tasks complete.
    virtual auto shutdown() noexcept -> void = 0;

    /// @brief Returns true if shutdown has been requested.
    [[nodiscard]] virtual auto is_shutdown() const -> bool = 0;

    /// @brief Number of tasks currently executing or queued.
    virtual auto size() const noexcept -> std::size_t = 0;

    /// @brief True if no tasks are executing or queued.
    [[nodiscard]] virtual auto empty() const noexcept -> bool = 0;

    /// @brief Number of tasks waiting in the queue.
    virtual auto queue_size() const noexcept -> std::size_t = 0;

    /// @brief True if the task queue is empty.
    [[nodiscard]] virtual auto queue_empty() const noexcept -> bool = 0;
};

/// @brief Concrete thread pool.
///
/// Uses the pimpl idiom: all data members live in pool::Impl, defined in the
/// module implementation unit (pool.cpp). Template convenience overloads
/// (schedule<return_type>, resume<range_type>) are defined inline here.
class pool final: public IPool {
    struct private_constructor {
        explicit private_constructor() = default;
    };

  public:
    class schedule_operation {
        friend class pool;
        explicit schedule_operation(pool &tp) noexcept;

      public:
        auto await_ready() noexcept -> bool { return false; }
        auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> void;
        auto await_resume() noexcept -> void {}

      private:
        pool &m_thread_pool;
    };

    struct options {
        uint32_t thread_count = std::thread::hardware_concurrency();
        std::function<void(std::size_t)> on_thread_start_functor = nullptr;
        std::function<void(std::size_t)> on_thread_stop_functor = nullptr;
    };

    explicit pool(options &&opts, private_constructor);

    static auto make_unique(
            options opts = options{
                    .thread_count = std::thread::hardware_concurrency(),
                    .on_thread_start_functor = nullptr,
                    .on_thread_stop_functor = nullptr
            }
    ) -> std::unique_ptr<pool>;

    pool(const pool &) = delete;
    pool(pool &&) = delete;
    auto operator=(const pool &) -> pool & = delete;
    auto operator=(pool &&) -> pool & = delete;

    ~pool() override;

    [[nodiscard]] auto thread_count() const noexcept -> std::size_t override;

    [[nodiscard]] auto schedule() -> schedule_operation;

    auto spawn_detached(silicon::task::task<void> &&task) noexcept -> bool override;
    auto spawn_joinable(silicon::task::task<void> &&task) noexcept -> silicon::task::task<void> override;

    template<typename return_type>
    [[nodiscard]] auto schedule(silicon::task::task<return_type> task) -> silicon::task::task<return_type> {
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

    auto queue_size() const noexcept -> std::size_t override;
    auto queue_empty() const noexcept -> bool override { return queue_size() == 0; }

  private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    auto resume_range_impl(std::vector<std::coroutine_handle<>> &handles) noexcept -> std::size_t;
};

} // namespace silicon::thread

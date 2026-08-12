module;

// 模块化补齐：原经传递 include 获得的标准头，模块单元须显式包含。
#include <memory>



#include <atomic>
#include <coroutine>
#include <mutex>
#include <utility>
#include <expected>
#include <system_error>


export module silicon.coroutine:mutex;
import silicon.scheduler.task;

export namespace silicon::coroutine {

/// 统一错误返回类型：coroutine 模块所有可失败 API 返回 coroutine::result<T>。
template<typename T>
using result = std::expected<T, std::error_code>;

/// coroutine 模块专属错误码枚举（同步原语与协程池）。
enum class coroutine_error {
    kNullExecutor = 1,
    kInvalidPoolSize,
    kAlreadyUnlocked,
    kUnknown,
};

/// coroutine::channel / queue / ring_buffer 专属错误码枚举。
enum class channel_error {
    kClosed = 1,
    kTimeout,
    kCancelled,
};

/// 返回 coroutine_error 专属 error_category（name() = "silicon.coroutine"）。
[[nodiscard]] const std::error_category &coroutine_category() noexcept;

/// 返回 channel_error 专属 error_category（name() = "silicon.channel"）。
[[nodiscard]] const std::error_category &channel_category() noexcept;

/// 将 coroutine_error 转为 std::error_code。
[[nodiscard]] std::error_code make_error_code(coroutine_error e) noexcept;

/// 将 channel_error 转为 std::error_code。
[[nodiscard]] std::error_code make_error_code(channel_error e) noexcept;
class mutex;
class scoped_lock;
class condition_variable;



struct lock_operation_base {
    explicit lock_operation_base(silicon::coroutine::mutex &m): m_mutex(m) {}
    virtual ~lock_operation_base() = default;

    lock_operation_base(const lock_operation_base &) = delete;
    lock_operation_base(lock_operation_base &&) = delete;
    auto operator=(const lock_operation_base &) -> lock_operation_base & = delete;
    auto operator=(lock_operation_base &&) -> lock_operation_base & = delete;

    auto await_ready() const noexcept -> bool;
    auto await_suspend(std::coroutine_handle<> awaiting_coroutine) noexcept -> bool;

    std::coroutine_handle<> m_awaiting_coroutine;
    lock_operation_base *m_next{nullptr};

  protected:
    friend class silicon::coroutine::mutex;

    silicon::coroutine::mutex &m_mutex;
};

template<typename return_type>
struct lock_operation: public lock_operation_base {
    explicit lock_operation(silicon::coroutine::mutex &m): lock_operation_base(m) {}
    ~lock_operation() override = default;

    lock_operation(const lock_operation &) = delete;
    lock_operation(lock_operation &&) = delete;
    auto operator=(const lock_operation &) -> lock_operation & = delete;
    auto operator=(lock_operation &&) -> lock_operation & = delete;

    auto await_resume() noexcept -> return_type {
        if constexpr(std::is_same_v<scoped_lock, return_type>) {
            return scoped_lock{this->m_mutex};
        } else {
            return;
        }
    }
};



/**
 * A scoped RAII lock holder similar to std::unique_lock.
 */
class scoped_lock {
    friend class silicon::coroutine::mutex;
    friend class silicon::coroutine::condition_variable; // cv.wait() functions need to be able do unlock and re-lock

  public:
    enum class lock_strategy {
        /// The lock is already acquired, adopt it as the new owner.
        kAdopt
    };

    explicit scoped_lock(class silicon::coroutine::mutex &m, lock_strategy strategy = lock_strategy::kAdopt);

    /**
     * Unlocks the mutex upon this shared lock destructing.
     */
    ~scoped_lock();

    scoped_lock(const scoped_lock &) = delete;
    scoped_lock(scoped_lock &&other) noexcept;
    auto operator=(const scoped_lock &) -> scoped_lock & = delete;
    auto operator=(scoped_lock &&other) noexcept -> scoped_lock &;

    /**
     * Unlocks the scoped lock prior to it going out of scope.
     */
    auto unlock() -> void;

  private:
    /// Implementation state, fully hidden in the implementation unit.
    struct Impl;
    std::unique_ptr<Impl> m_p;

    /**
     * @brief Non-template accessor for the currently owned mutex.
     *
     * silicon::coroutine::condition_variable has to unlock and re-lock the caller's mutex from both
     * implementation-unit code and templated wait hooks living in the interface unit.  Routing those
     * accesses through this non-template hook keeps Impl fully hidden in mutex.cpp.
     *
     * @return The owned mutex, or nullptr if the lock has already been released.
     */
    [[nodiscard]] auto owned_mutex() const noexcept -> class silicon::coroutine::mutex *;
};

class mutex {
  public:
    explicit mutex() noexcept;
    ~mutex();

    mutex(const mutex &) = delete;
    mutex(mutex &&) = delete;
    auto operator=(const mutex &) -> mutex & = delete;
    auto operator=(mutex &&) -> mutex & = delete;

    /**
     * @brief To acquire the mutex's lock co_await this function. Upon acquiring the lock it returns a silicon::coroutine::scoped_lock
     *        which will hold the mutex until the silicon::coroutine::scoped_lock destructs.
     * @return A co_await'able operation to acquire the mutex.
     */
    [[nodiscard]] auto scoped_lock() -> lock_operation<scoped_lock> { return lock_operation<silicon::coroutine::scoped_lock>{*this}; }

    /**
     * @brief Locks the mutex.
     *
     * @return lock_operation<void>
     */
    [[nodiscard]] auto lock() -> lock_operation<void> { return lock_operation<void>{*this}; }

    /**
     * Attempts to lock the mutex.
     * @return True if the mutex lock was acquired, otherwise false.
     */
    [[nodiscard]] auto try_lock() -> bool;

    /**
     * Releases the mutex's lock.
     * @return coroutine::result<void>；重复解锁（逻辑错误）时返回
     *         std::unexpected(coroutine_error::kAlreadyUnlocked)。
     */
    auto unlock() -> result<void>;

  private:
    friend struct lock_operation_base;

    /// Implementation state, fully hidden in the implementation unit.
    /// unlocked -> state == unlocked_value()
    /// locked but empty waiter list == nullptr
    /// locked with waiters == lock_operation_base*
    struct Impl;
    std::unique_ptr<Impl> m_p;

    /// Inactive value, this cannot be nullptr since we want nullptr to signify that the mutex
    /// is locked but there are zero waiters, this makes it easy to CAS new waiters into the
    /// m_state linked list.
    auto unlocked_value() const noexcept -> const void *;
};

} // namespace silicon::coroutine

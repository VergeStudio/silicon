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
export import silicon.coroutine.error;
import silicon.scheduler.task;
import silicon.error;

export namespace silicon::coroutine {

/// 统一错误返回类型：coroutine 模块所有可失败 API 返回 coroutine::result<T>。
/// 转发至 silicon.error 的集中别名。
template<typename T>
using result = silicon::error::result<T>;

class mutex;
class scoped_lock;
class condition_variable;



struct lock_operation_base {
    explicit lock_operation_base(silicon::coroutine::mutex &m): m_mutex(m) {}
    virtual ~lock_operation_base() = default;

    lock_operation_base(const lock_operation_base &) = delete;
    lock_operation_base(lock_operation_base &&) = delete;
    lock_operation_base & operator=(const lock_operation_base &) = delete;
    lock_operation_base & operator=(lock_operation_base &&) = delete;

    bool await_ready() const noexcept ;
    bool await_suspend(std::coroutine_handle<>) noexcept ;

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
    lock_operation & operator=(const lock_operation &) = delete;
    lock_operation & operator=(lock_operation &&) = delete;

    return_type await_resume() noexcept {
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
    scoped_lock & operator=(const scoped_lock &) = delete;
    scoped_lock & operator=(scoped_lock &&other) noexcept ;

    /**
     * Unlocks the scoped lock prior to it going out of scope.
     */
    void unlock() ;

  private:
    /// Implementation state, fully hidden in the implementation unit.
    struct impl;
    std::unique_ptr<impl> m_p;

    /**
     * @brief Non-template accessor for the currently owned mutex.
     *
     * silicon::coroutine::condition_variable has to unlock and re-lock the caller's mutex from both
     * implementation-unit code and templated wait hooks living in the interface unit.  Routing those
     * accesses through this non-template hook keeps impl fully hidden in mutex.cpp.
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
    mutex & operator=(const mutex &) = delete;
    mutex & operator=(mutex &&) = delete;

    /**
     * @brief To acquire the mutex's lock co_await this function. Upon acquiring the lock it returns a silicon::coroutine::scoped_lock
     *        which will hold the mutex until the silicon::coroutine::scoped_lock destructs.
     * @return A co_await'able operation to acquire the mutex.
     */
    [[nodiscard]] lock_operation<scoped_lock> scoped_lock() { return lock_operation<silicon::coroutine::scoped_lock>{*this}; }

    /**
     * @brief Locks the mutex.
     *
     * @return lock_operation<void>
     */
    [[nodiscard]] lock_operation<void> lock() { return lock_operation<void>{*this}; }

    /**
     * Attempts to lock the mutex.
     * @return True if the mutex lock was acquired, otherwise false.
     */
    [[nodiscard]] bool try_lock() ;

    /**
     * Releases the mutex's lock.
     * @return coroutine::result<void>；重复解锁（逻辑错误）时返回
     *         std::unexpected(coroutine_error::kAlreadyUnlocked)。
     */
    result<void> unlock() ;

  private:
    friend struct lock_operation_base;

    /// Implementation state, fully hidden in the implementation unit.
    /// unlocked -> state == unlocked_value()
    /// locked but empty waiter list == nullptr
    /// locked with waiters == lock_operation_base*
    struct impl;
    std::unique_ptr<impl> m_p;

    /// Inactive value, this cannot be nullptr since we want nullptr to signify that the mutex
    /// is locked but there are zero waiters, this makes it easy to CAS new waiters into the
    /// m_state linked list.
    const void * unlocked_value() const noexcept ;
};

} // namespace silicon::coroutine

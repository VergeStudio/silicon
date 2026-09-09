module;

#include <memory>

#include <atomic>
#include <coroutine>
#include <mutex>
#include <utility>
#include <expected>
#include <system_error>

#include "silicon/common.h"

export module silicon.coroutine:mutex;
export import silicon.coroutine.error;
import silicon.scheduler.task;
import silicon.error;

export namespace silicon::coroutine {

class mutex;
class scoped_lock;
class condition_variable;

struct SILICON_CORE_API lock_operation_base {
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

class SILICON_CORE_API scoped_lock {
    friend class silicon::coroutine::mutex;
    friend class silicon::coroutine::condition_variable;

  public:
    enum class lock_strategy {

        kAdopt
    };

    explicit scoped_lock(class silicon::coroutine::mutex &, lock_strategy = lock_strategy::kAdopt);

    ~scoped_lock();

    scoped_lock(const scoped_lock &) = delete;
    scoped_lock(scoped_lock &&other) noexcept;
    scoped_lock & operator=(const scoped_lock &) = delete;
    scoped_lock & operator=(scoped_lock &&other) noexcept ;

    void unlock() ;

  private:

    struct impl;
    std::unique_ptr<impl> m_p;

    [[nodiscard]] auto owned_mutex() const noexcept -> class silicon::coroutine::mutex *;
};

class SILICON_CORE_API mutex {
  public:
    explicit mutex() noexcept;
    ~mutex();

    mutex(const mutex &) = delete;
    mutex(mutex &&) = delete;
    mutex & operator=(const mutex &) = delete;
    mutex & operator=(mutex &&) = delete;

    [[nodiscard]] lock_operation<scoped_lock> scoped_lock() { return lock_operation<silicon::coroutine::scoped_lock>{*this}; }

    [[nodiscard]] lock_operation<void> lock() { return lock_operation<void>{*this}; }

    [[nodiscard]] bool try_lock() ;

    silicon::error::result<void> unlock() ;

  private:
    friend struct lock_operation_base;

    struct impl;
    std::unique_ptr<impl> m_p;

    const void * unlocked_value() const noexcept ;
};

}

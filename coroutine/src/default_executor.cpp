module;
#include <atomic>
#include <memory>

module silicon.coroutine;



static const auto s_initialization_check_interval = std::chrono::milliseconds(1);

static silicon::coroutine::pool::options s_default_executor_options;
static std::atomic<bool> s_default_executor_init{false};
static std::atomic<silicon::coroutine::pool *> s_default_executor_ptr{nullptr};
static std::unique_ptr<silicon::coroutine::pool> s_default_executor{nullptr};

#ifdef LIBCORO_FEATURE_NETWORKING
static silicon::coroutine::scheduler::options s_default_io_executor_options;
static std::atomic<bool> s_default_io_executor_init{false};
static std::atomic<silicon::coroutine::scheduler *> s_default_io_executor_ptr{nullptr};
static std::unique_ptr<silicon::coroutine::scheduler> s_default_io_executor;
#endif

void silicon::coroutine::default_executor::set_executor_options(pool::options pool_options) {
    s_default_executor_options = std::move(pool_options);
}

std::unique_ptr<silicon::coroutine::pool> &silicon::coroutine::default_executor::executor() {
    // If we're the first one here create the default executor.
    if(s_default_executor_init.exchange(true) == false) {
        s_default_executor = silicon::coroutine::pool::make_unique(s_default_executor_options);
        s_default_executor_ptr.store(s_default_executor.get(), std::memory_order::release);
    }

    while(s_default_executor_ptr.load(std::memory_order::acquire) == nullptr) {
        std::this_thread::sleep_for(s_initialization_check_interval);
    }

    return s_default_executor;
}

#ifdef LIBCORO_FEATURE_NETWORKING
void silicon::coroutine::default_executor::set_io_executor_options(scheduler::options scheduler_options) {
    s_default_io_executor_options = std::move(scheduler_options);
}

std::unique_ptr<silicon::coroutine::scheduler> &silicon::coroutine::default_executor::io_executor() {
    // If we're the first one here create the default executor.
    if(s_default_io_executor_init.exchange(true) == false) {
        s_default_io_executor = silicon::coroutine::scheduler::make_unique(s_default_io_executor_options);
        s_default_io_executor_ptr.store(s_default_io_executor.get(), std::memory_order::release);
    }

    while(s_default_io_executor_ptr.load(std::memory_order::acquire) == nullptr) {
        std::this_thread::sleep_for(s_initialization_check_interval);
    }

    return s_default_io_executor;
}
#endif

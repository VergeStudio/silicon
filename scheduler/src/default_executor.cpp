module;
// 迁出 silicon.coroutine 后不再借道该模块 GMF 间接获得 <chrono>/<thread>，
// 此处显式引入（s_initialization_check_interval / std::this_thread::sleep_for）。
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

module silicon.scheduler;



static const auto s_initialization_check_interval = std::chrono::milliseconds(1);

static silicon::scheduler::thread_pool::options s_default_executor_options;
static std::atomic<bool> s_default_executor_init{false};
static std::atomic<silicon::scheduler::thread_pool *> s_default_executor_ptr{nullptr};
static std::unique_ptr<silicon::scheduler::thread_pool> s_default_executor{nullptr};

#ifdef LIBCORO_FEATURE_NETWORKING
static silicon::scheduler::io_scheduler::options s_default_io_executor_options;
static std::atomic<bool> s_default_io_executor_init{false};
static std::atomic<silicon::scheduler::io_scheduler *> s_default_io_executor_ptr{nullptr};
static std::unique_ptr<silicon::scheduler::io_scheduler> s_default_io_executor;
#endif

void silicon::scheduler::default_executor::set_executor_options(thread_pool::options pool_options) {
    s_default_executor_options = std::move(pool_options);
}

std::unique_ptr<silicon::scheduler::thread_pool> &silicon::scheduler::default_executor::executor() {
    // If we're the first one here create the default executor.
    if(s_default_executor_init.exchange(true) == false) {
        s_default_executor = silicon::scheduler::thread_pool::make_unique(s_default_executor_options);
        s_default_executor_ptr.store(s_default_executor.get(), std::memory_order::release);
    }

    while(s_default_executor_ptr.load(std::memory_order::acquire) == nullptr) {
        std::this_thread::sleep_for(s_initialization_check_interval);
    }

    return s_default_executor;
}

#ifdef LIBCORO_FEATURE_NETWORKING
void silicon::scheduler::default_executor::set_io_executor_options(io_scheduler::options scheduler_options) {
    s_default_io_executor_options = std::move(scheduler_options);
}

std::unique_ptr<silicon::scheduler::io_scheduler> &silicon::scheduler::default_executor::io_executor() {
    // If we're the first one here create the default executor.
    if(s_default_io_executor_init.exchange(true) == false) {
        s_default_io_executor = silicon::scheduler::io_scheduler::make_unique(s_default_io_executor_options);
        s_default_io_executor_ptr.store(s_default_io_executor.get(), std::memory_order::release);
    }

    while(s_default_io_executor_ptr.load(std::memory_order::acquire) == nullptr) {
        std::this_thread::sleep_for(s_initialization_check_interval);
    }

    return s_default_io_executor;
}
#endif

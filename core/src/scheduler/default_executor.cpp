module;

#include <atomic>
#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <system_error>
#include <utility>
#include <coroutine>
#include <map>
#include <optional>

module silicon.scheduler;

import :poll_info_impl;
import silicon.time;

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

    if(s_default_executor_init.exchange(true) == false) {
        auto created = silicon::scheduler::thread_pool::create(s_default_executor_options);
        if(!created) {
            std::cerr << "silicon::scheduler: failed to create default thread pool: "
                      << created.error().message() << "\n";
            std::terminate();
        }
        s_default_executor = std::move(*created);
        s_default_executor_ptr.store(s_default_executor.get(), std::memory_order::release);
    }

    while(s_default_executor_ptr.load(std::memory_order::acquire) == nullptr) {
        silicon::time::sleep_for(s_initialization_check_interval);
    }

    return s_default_executor;
}

#ifdef LIBCORO_FEATURE_NETWORKING
void silicon::scheduler::default_executor::set_io_executor_options(io_scheduler::options scheduler_options) {
    s_default_io_executor_options = std::move(scheduler_options);
}

std::unique_ptr<silicon::scheduler::io_scheduler> &silicon::scheduler::default_executor::io_executor() {

    if(s_default_io_executor_init.exchange(true) == false) {
        auto ios = silicon::scheduler::io_scheduler::create(s_default_io_executor_options);
        if(!ios) {
            std::cerr << "silicon::scheduler::default_executor: failed to create default io_scheduler ("
                      << ios.error().message() << ")\n";
            std::terminate();
        }
        s_default_io_executor = std::move(*ios);
        s_default_io_executor_ptr.store(s_default_io_executor.get(), std::memory_order::release);
    }

    while(s_default_io_executor_ptr.load(std::memory_order::acquire) == nullptr) {
        silicon::time::sleep_for(s_initialization_check_interval);
    }

    return s_default_io_executor;
}
#endif

module;

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <thread>
#include <coroutine>
#include <map>
#include <optional>

module silicon.scheduler;

import :poll_info_impl;

namespace silicon::scheduler {



struct parallel_scheduler::impl {
    std::unique_ptr<thread_pool> m_pool;
};

parallel_scheduler::parallel_scheduler(): m_impl(std::make_unique<impl>()) {
    m_impl->m_pool = thread_pool::create(
            thread_pool::options{
                    .thread_count = std::thread::hardware_concurrency(),
            }).value();
}

parallel_scheduler::~parallel_scheduler() {

    shutdown();
}

std::size_t parallel_scheduler::thread_count() const noexcept {
    return m_impl->m_pool->thread_count();
}

bool parallel_scheduler::spawn_detached(task<void> &&task) noexcept {
    return m_impl->m_pool->spawn_detached(std::move(task));
}

auto parallel_scheduler::spawn_joinable(task<void> &&t) noexcept -> task<void> {
    return m_impl->m_pool->spawn_joinable(std::move(t));
}

bool parallel_scheduler::resume(std::coroutine_handle<> handle) noexcept {
    return m_impl->m_pool->resume(handle);
}

void parallel_scheduler::shutdown() noexcept {
    m_impl->m_pool->shutdown();
}

bool parallel_scheduler::is_shutdown() const {
    return m_impl->m_pool->is_shutdown();
}

std::size_t parallel_scheduler::size() const noexcept {
    return m_impl->m_pool->size();
}

auto parallel_scheduler::get_parallel_scheduler() -> parallel_scheduler & {
    static parallel_scheduler instance{};
    return instance;
}

}

module;

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <thread>

module silicon.scheduler;

namespace silicon::scheduler {

// parallel_scheduler 通过组合持有底层 thread_pool，并把全部 i_scheduler 操作委托给它。
// 自身不做任何队列 / 线程管理，仅承担 "系统并行调度器" 这一语义角色。
struct parallel_scheduler::Impl {
    std::unique_ptr<thread_pool> m_pool;
};

parallel_scheduler::parallel_scheduler(): m_impl(std::make_unique<Impl>()) {
    m_impl->m_pool = thread_pool::create(
            thread_pool::options{
                    .thread_count = std::thread::hardware_concurrency(),
            }).value();
}

parallel_scheduler::~parallel_scheduler() {
    // 委托底层线程池的 shutdown()：阻塞 join 所有工作线程。
    shutdown();
}

auto parallel_scheduler::thread_count() const noexcept -> std::size_t {
    return m_impl->m_pool->thread_count();
}

auto parallel_scheduler::spawn_detached(task::task<void> &&task) noexcept -> bool {
    return m_impl->m_pool->spawn_detached(std::move(task));
}

auto parallel_scheduler::spawn_joinable(task::task<void> &&task) noexcept -> task::task<void> {
    return m_impl->m_pool->spawn_joinable(std::move(task));
}

auto parallel_scheduler::resume(std::coroutine_handle<> handle) noexcept -> bool {
    return m_impl->m_pool->resume(handle);
}

auto parallel_scheduler::shutdown() noexcept -> void {
    m_impl->m_pool->shutdown();
}

auto parallel_scheduler::is_shutdown() const -> bool {
    return m_impl->m_pool->is_shutdown();
}

auto parallel_scheduler::size() const noexcept -> std::size_t {
    return m_impl->m_pool->size();
}

auto parallel_scheduler::get_parallel_scheduler() -> parallel_scheduler & {
    static parallel_scheduler instance{};
    return instance;
}

} // namespace silicon::scheduler

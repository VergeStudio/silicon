module;

// 模块化补齐：实现单元自给标准头（模块接口不向其传递）。
#include <atomic>
#include <coroutine>
#include <cstdint>
#include <expected>
#include <system_error>
#include <utility>

module silicon.scheduler;
// MSVC 须显式 import 本模块接口方可访问其导出实体；clang 与标准不允许
// 实现单元自引用，故以 _MSC_VER 守卫。
#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_op;

// completion I/O 操作（io_op）的 out-of-line 成员定义。类型在 :io_op 分区
// 声明且不导出（模块内部链接），因此本单元无需 CORE_API。

namespace silicon::scheduler {

void io_op::complete(std::int64_t transfer) noexcept {
    m_transfer = transfer;
    m_error = {};
}

void io_op::complete_error(std::error_code ec) noexcept {
    m_transfer = 0;
    m_error = ec;
}

auto io_op::result() const noexcept -> std::expected<std::int64_t, std::error_code> {
    if(m_error) {
        return std::unexpected(m_error);
    }
    return m_transfer;
}

void io_op::io_awaiter::await_suspend(std::coroutine_handle<> awaiting) noexcept {
    // 与 poll_info::poll_awaiter::await_suspend 同款发布语义：驱动线程在
    // drain_ring_completions 中以 acquire fence 自旋等待该句柄可见后再恢复。
    m_op.m_awaiting_coroutine = awaiting;
    std::atomic_thread_fence(std::memory_order::release);
}

auto io_op::io_awaiter::await_resume() noexcept -> std::expected<std::int64_t, std::error_code> {
    return m_op.result();
}

} // namespace silicon::scheduler

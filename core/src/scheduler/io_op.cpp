module;

#include <atomic>
#include <coroutine>
#include <cstdint>
#include <expected>
#include <system_error>
#include <utility>

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_op;

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

    m_op.m_awaiting_coroutine = awaiting;
    std::atomic_thread_fence(std::memory_order::release);
}

auto io_op::io_awaiter::await_resume() noexcept -> std::expected<std::int64_t, std::error_code> {
    return m_op.result();
}

}

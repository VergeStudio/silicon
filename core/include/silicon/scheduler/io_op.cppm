module;

#include <atomic>
#include <coroutine>
#include <cstdint>
#include <expected>
#include <system_error>
#include <utility>

export module silicon.scheduler:io_op;

import :fd;

namespace silicon::scheduler {

struct io_op {

    silicon::scheduler::fd_t m_fd{-1};

    void *m_buffer{nullptr};

    std::uint32_t m_length{0};

    std::uint64_t m_offset{0};

    bool m_is_write{false};

    std::coroutine_handle<> m_awaiting_coroutine{nullptr};

    bool m_processed{false};

    std::int64_t m_transfer{0};

    std::error_code m_error{};

    io_op *m_next{nullptr};

    void complete(std::int64_t) noexcept ;

    void complete_error(std::error_code) noexcept ;

    [[nodiscard]] std::expected<std::int64_t, std::error_code> result() const noexcept ;

    struct io_awaiter {
        explicit io_awaiter(io_op &op) noexcept: m_op(op) {}
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> awaiting) noexcept ;
        std::expected<std::int64_t, std::error_code> await_resume() noexcept ;

        io_op &m_op;
    };

    io_awaiter operator co_await() noexcept { return io_awaiter{*this}; }
};

}

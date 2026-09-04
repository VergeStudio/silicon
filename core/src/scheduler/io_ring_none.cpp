module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

module silicon.scheduler;

#if defined(_MSC_VER)
import silicon.scheduler;
#endif

import :io_ring;

#if !defined(SILICON_FEATURE_IO_RING) ||                                       \
    (!defined(SILICON_PLATFORM_WINDOWS) && !defined(SILICON_PLATFORM_LINUX))

namespace silicon::scheduler {

struct io_ring::impl {
    io_ring_config cfg{};
};

io_ring::io_ring(io_ring_config cfg): m_p(std::make_unique<impl>()) {
    m_p->cfg = cfg;
}

io_ring::~io_ring() = default;

bool io_ring::is_valid() const noexcept { return false; }

auto io_ring::active_backend() const noexcept -> backend { return backend::none; }

bool io_ring::supports(op) const noexcept { return false; }

bool io_ring::submit_read(fd_t, void *, std::uint32_t, std::uint64_t, std::uint64_t) {
    return false;
}

bool io_ring::submit_write(fd_t, const void *, std::uint32_t, std::uint64_t, std::uint64_t) {
    return false;
}

bool io_ring::submit_cancel(std::uint64_t, std::uint64_t) { return false; }

std::uint32_t io_ring::submit() { return 0; }

auto io_ring::wait_completion(std::chrono::milliseconds) -> std::optional<completion> {
    return std::nullopt;
}

auto io_ring::peek_completion() -> std::optional<completion> { return std::nullopt; }

}

#endif

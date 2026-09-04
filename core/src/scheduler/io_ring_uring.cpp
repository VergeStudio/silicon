module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#if defined(SILICON_PLATFORM_LINUX) && defined(SILICON_FEATURE_IO_RING)
#    include <linux/time_types.h>
#    include <sys/types.h>
#    include <liburing.h>
#endif

module silicon.scheduler;

#if defined(SILICON_PLATFORM_LINUX) && defined(SILICON_FEATURE_IO_RING)

namespace silicon::scheduler {

struct io_ring::impl {
    struct io_uring ring{};
    bool valid{false};
};

io_ring::io_ring(io_ring_config cfg): m_p(std::make_unique<impl>()) {
    unsigned depth = 1u;
    while(depth < cfg.queue_depth && depth < 4096u) { depth <<= 1; }

    struct io_uring_params params{};
    if(cfg.sq_poll) { params.flags |= IORING_SETUP_SQPOLL; }

    m_p->valid = (io_uring_queue_init_params(depth, &m_p->ring, &params) == 0);
}

io_ring::~io_ring() {
    if(m_p && m_p->valid) {
        io_uring_queue_exit(&m_p->ring);
        m_p->valid = false;
    }
}

bool io_ring::is_valid() const noexcept { return m_p && m_p->valid; }

auto io_ring::active_backend() const noexcept -> backend {
    return is_valid() ? backend::io_uring : backend::none;
}

bool io_ring::supports(op) const noexcept { return is_valid(); }

bool io_ring::submit_read(fd_t fd, void *buf, std::uint32_t len, std::uint64_t offset, std::uint64_t user_data) {
    if(!is_valid()) { return false; }

    auto *sqe = io_uring_get_sqe(&m_p->ring);
    if(sqe == nullptr) { return false; }

    io_uring_prep_read(sqe, fd, buf, len, static_cast<off_t>(offset));
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(user_data));
    return true;
}

bool io_ring::submit_write(
        fd_t fd, const void *buf, std::uint32_t len, std::uint64_t offset, std::uint64_t user_data
) {
    if(!is_valid()) { return false; }

    auto *sqe = io_uring_get_sqe(&m_p->ring);
    if(sqe == nullptr) { return false; }

    io_uring_prep_write(sqe, fd, buf, len, static_cast<off_t>(offset));
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(user_data));
    return true;
}

bool io_ring::submit_cancel(std::uint64_t target_user_data, std::uint64_t user_data) {
    if(!is_valid()) { return false; }

    auto *sqe = io_uring_get_sqe(&m_p->ring);
    if(sqe == nullptr) { return false; }

    io_uring_prep_cancel(sqe, reinterpret_cast<void *>(target_user_data), 0);
    io_uring_sqe_set_data(sqe, reinterpret_cast<void *>(user_data));
    return true;
}

std::uint32_t io_ring::submit() {
    if(!is_valid()) { return 0u; }

    int submitted = io_uring_submit(&m_p->ring);
    return submitted > 0 ? static_cast<std::uint32_t>(submitted) : 0u;
}

auto io_ring::wait_completion(std::chrono::milliseconds timeout) -> std::optional<completion> {
    if(!is_valid()) { return std::nullopt; }

    struct io_uring_cqe *cqe = nullptr;
    struct __kernel_timespec ts{};
    ts.tv_sec = static_cast<decltype(ts.tv_sec)>(timeout.count() / 1000);
    ts.tv_nsec = (timeout.count() % 1000) * 1000000LL;

    if(io_uring_wait_cqe_timeout(&m_p->ring, &cqe, &ts) != 0 || cqe == nullptr) {
        return std::nullopt;
    }

    completion done{
            reinterpret_cast<std::uint64_t>(io_uring_cqe_get_data(cqe)),
            cqe->res,
            cqe->flags
    };
    io_uring_cqe_seen(&m_p->ring, cqe);
    return done;
}

auto io_ring::peek_completion() -> std::optional<completion> {
    if(!is_valid()) { return std::nullopt; }

    struct io_uring_cqe *cqe = nullptr;
    if(io_uring_peek_cqe(&m_p->ring, &cqe) != 0 || cqe == nullptr) { return std::nullopt; }

    completion done{
            reinterpret_cast<std::uint64_t>(io_uring_cqe_get_data(cqe)),
            cqe->res,
            cqe->flags
    };
    io_uring_cqe_seen(&m_p->ring, cqe);
    return done;
}

}

#endif

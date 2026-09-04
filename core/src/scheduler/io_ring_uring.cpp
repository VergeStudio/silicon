// Linux 后端：io_uring（liburing）。
// 仅当 xmake 选项 --io_ring=y 且平台为 Linux 时参与构建；否则本文件编译为空
// TU。守卫与 io_ring_ioring.cpp 的 Windows 守卫互斥，恰好一个文件定义同组符号。
//
// 平台无关的 io_ring 接口在 :io_ring 分区；本文件只提供 impl 实体与各方法的
// Linux 实现。未启用时消费方应回退 :io_notifier 的 epoll 路径。

module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

#if defined(SILICON_PLATFORM_LINUX) && defined(SILICON_FEATURE_IO_RING)
#    include <linux/time_types.h> // __kernel_timespec（io_uring_wait_cqe_timeout）
#    include <sys/types.h>        // off_t
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

    // 初始化失败不抛异常：交由 is_valid() 表达，消费方回退 epoll。
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

// io_uring 覆盖本类全部操作；个别 op 的内核门槛（如 IORING_OP_ASYNC_CANCEL
// 需 5.5+）由提交失败时的 CQE 负 errno 体现，此处不重复做版本探测。
bool io_ring::supports(op) const noexcept { return is_valid(); }

bool io_ring::submit_read(fd_t fd, void *buf, std::uint32_t len, std::uint64_t offset, std::uint64_t user_data) {
    if(!is_valid()) { return false; }

    auto *sqe = io_uring_get_sqe(&m_p->ring);
    if(sqe == nullptr) { return false; } // SQ 已满：调用方应先 submit()

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
        return std::nullopt; // 超时或被信号中断
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

} // namespace silicon::scheduler

#endif // SILICON_PLATFORM_LINUX && SILICON_FEATURE_IO_RING

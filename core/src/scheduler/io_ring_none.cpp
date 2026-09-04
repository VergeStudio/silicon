// io_ring 的"无后端"实现单元。
//
// 平台后端各自实现在 io_ring_uring.cpp（Linux=io_uring）与 io_ring_ioring.cpp
//（Windows=I/O Ring），二者**只在**「--io_ring=y 且平台匹配」时编入符号。
// 而在没有任何后端参与构建的配置下（默认构建、或 mac/BSD 等无后端平台），
// io_ring 的成员就全部没有定义——但 :io_ring 分区对外**无条件**声明了这套
// 接口，io_scheduler 的 options 亦持有 io_ring_config，链接期因此报 LNK2001：
// io_ring::io_ring(io_ring_config) 未解析，且几乎所有 import 本模块的 TU 都会
// 命中（分区内的实体被实例化到每个 import 方）。
//
// 本文件补上这一环：提供全部成员的空实现，令 io_ring 在任何配置下都可链接，
// 且行为与 :io_ring 分区接口的承诺一致——is_valid() 恒 false、active_backend()
// 恒 none、supports() 恒 false、提交恒失败、收割恒返回空。消费方据
// completion_backend() == none 回退到 :io_notifier 的 readiness 路径。
//
// 守卫与两个平台后端文件互斥：未定义 SILICON_FEATURE_IO_RING，或平台既非
// Windows 也非 Linux 时，本文件是唯一定义 io_ring 符号的 TU。

module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>

module silicon.scheduler;
// MSVC 须显式 import 本模块接口方可访问其导出实体；clang 与标准不允许
// 实现单元自引用，故以 _MSC_VER 守卫。
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

} // namespace silicon::scheduler

#endif // 无可用后端

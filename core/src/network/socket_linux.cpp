// POSIX/BSD·Linux implementation of the platform-specific socket members.
// 平台无关的 socket 实现（type_to_os / 移动赋值 / make_socket /
// make_accept_socket）在公共实现单元 socket.cpp；本文件仅提供句柄语义随平台
// 变化的成员，以及 socket_duplicate_handle / socket_enable_address_reuse
// 两个辅助。守卫与 socket_win.cpp 的 SILICON_PLATFORM_WINDOWS 守卫互斥，
// 恰好一个文件定义同组符号。

module;

#if defined(SILICON_PLATFORM_UNIX)
#    include <arpa/inet.h>
#    include <fcntl.h>
#    include <sys/socket.h>
#    include <unistd.h>
#endif

#include <cerrno>
#include <system_error>

module silicon.network;
// MSVC 须显式 import 本模块接口方可访问其导出实体；clang 与标准不允许
// 实现单元自引用，故以 _MSC_VER 守卫。
#if defined(_MSC_VER)
import silicon.network;
#endif

import silicon.coroutine;

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::network {

int socket_duplicate_handle(int fd) { return ::dup(fd); }

bool socket_enable_address_reuse(int fd) {
    int sock_opt{1};

#if defined(SILICON_PLATFORM_LINUX)
    // On Linux the address and port should be marked for reuse.
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, static_cast<int>(sizeof(sock_opt))) < 0) {
        return false;
    }
#endif

    // SO_REUSEPORT is a BSD/Linux socket option; Windows has no equivalent
    // (SO_REUSEADDR already covers the port-reuse semantics there).
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &sock_opt, static_cast<int>(sizeof(sock_opt))) < 0) {
        return false;
    }

    return true;
}

auto socket::operator=(const socket &other) noexcept -> socket & {
    this->close();
    this->m_fd = socket_duplicate_handle(other.m_fd);
    return *this;
}

bool socket::blocking(blocking_t block) {
    if(m_fd < 0) {
        return false;
    }

    int flags = fcntl(m_fd, F_GETFL, 0);
    if(flags == -1) {
        return false;
    }

    // Add or subtract non-blocking flag.
    flags = (block == blocking_t::yes) ? flags & ~O_NONBLOCK : (flags | O_NONBLOCK);

    return (fcntl(m_fd, F_SETFL, flags) == 0);
}

bool socket::shutdown(silicon::coroutine::poll_op how) {
    if(m_fd != -1) {
        int h{0};
        switch(how) {
            case silicon::coroutine::poll_op::read:
                h = SHUT_RD;
                break;
            case silicon::coroutine::poll_op::write:
                h = SHUT_WR;
                break;
            case silicon::coroutine::poll_op::read_write:
                h = SHUT_RDWR;
                break;
        }

        return (::shutdown(m_fd, h) == 0);
    }
    return false;
}

void socket::close() {
    if(m_fd != -1) {
        ::close(m_fd);
        m_fd = -1;
    }
}

auto socket::accept(socket_address &client_endpoint) -> socket {
    // accept()/recvfrom() 需要可写缓冲区：必用 native_mutable_data()（返回
    // sockaddr* + socklen_t*），只读的 data() 返回 const sockaddr* + 值长度，不适用。
    auto [addr, addrlen] = client_endpoint.native_mutable_data();

    return socket{::accept(m_fd, addr, addrlen)};
}

int socket::last_error() const { return errno; }

int socket::connect(const socket_address &endpoint) {
    auto [addr, addrlen] = endpoint.data();

    return ::connect(m_fd, const_cast<sockaddr *>(addr), addrlen);
}

bool socket::in_progress() const {
    // A non-blocking connect() that has not yet completed returns EINPROGRESS
    // on POSIX or WSAEWOULDBLOCK on Windows; either way the connection is
    // establishing asynchronously and the caller should poll for writability.
    return (last_error() == EINPROGRESS);
}

} // namespace silicon::network

#endif // defined(SILICON_PLATFORM_UNIX)

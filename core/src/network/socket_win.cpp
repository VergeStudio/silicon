// Windows implementation of the platform-specific socket members.
// 平台无关的 socket 实现（type_to_os / 移动赋值 / make_socket /
// make_accept_socket）在公共实现单元 socket.cpp；本文件仅提供句柄语义随平台
// 变化的成员，以及 socket_duplicate_handle / socket_enable_address_reuse
// 两个辅助。守卫与 socket_linux.cpp 的 SILICON_PLATFORM_UNIX 守卫互斥，
// 恰好一个文件定义同组符号。

module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#endif

#include <system_error>

module silicon.network;
// MSVC 须显式 import 本模块接口方可访问其导出实体；clang 与标准不允许
// 实现单元自引用，故以 _MSC_VER 守卫。
#if defined(_MSC_VER)
import silicon.network;
#endif

import silicon.coroutine;

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::network {

int socket_duplicate_handle(int fd) {
    // Windows has no dup() for SOCKET handles; shallow-copy the handle.
    return fd;
}

bool socket_enable_address_reuse(int /*fd*/) {
    // 刻意不设置任何选项。Winsock 的 SO_REUSEADDR 与 POSIX 语义**不同**：
    // 在 Windows 上它允许**任意进程**绑定同一监听端口（端口劫持），是已知的安全
    // 陷阱；而 POSIX 的 SO_REUSEADDR 仅用于复用 TIME_WAIT 状态的地址。
    //
    // 按平台拆分之前，socket.cpp 中的 Windows 分支同样不调用 setsockopt
    //（Linux 设 SO_REUSEADDR、非 Windows 设 SO_REUSEPORT，Windows 两者皆无），
    // 此处保持该行为不变——重构不应改变语义。
    //
    // 形参以无名形式保留，使签名与 POSIX 版（socket_linux.cpp）一致。
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

    // Windows has no fcntl; non-blocking mode is controlled via ioctlsocket(FIONBIO).
    unsigned long mode = (block == blocking_t::yes) ? 0u : 1u;
    return (ioctlsocket(m_fd, FIONBIO, &mode) == 0);
}

bool socket::shutdown(silicon::scheduler::poll_op how) {
    if(m_fd != -1) {
        int h{0};
        switch(how) {
            case silicon::scheduler::poll_op::read:
                h = SD_RECEIVE;
                break;
            case silicon::scheduler::poll_op::write:
                h = SD_SEND;
                break;
            case silicon::scheduler::poll_op::read_write:
                h = SD_BOTH;
                break;
        }

        return (::shutdown(m_fd, h) == 0);
    }
    return false;
}

void socket::close() {
    if(m_fd != -1) {
        ::closesocket(m_fd);
        m_fd = -1;
    }
}

auto socket::accept(socket_address &client_endpoint) -> socket {
    // accept()/recvfrom() 需要可写缓冲区：必用 native_mutable_data()（返回
    // sockaddr* + socklen_t*），只读的 data() 返回 const sockaddr* + 值长度，不适用。
    auto [addr, addrlen] = client_endpoint.native_mutable_data();

    // Winsock's accept() takes an int* for addrlen and returns a SOCKET handle.
    int len = static_cast<int>(*addrlen);
    auto raw = ::accept(m_fd, addr, &len);
    *addrlen = static_cast<socklen_t>(len);
    return socket{static_cast<int>(raw)};
}

int socket::last_error() const { return static_cast<int>(WSAGetLastError()); }

int socket::connect(const socket_address &endpoint) {
    auto [addr, addrlen] = endpoint.data();

    // Winsock's connect() takes an int for addrlen and returns SOCKET_ERROR (-1)
    // on failure (check last_error() == WSAEWOULDBLOCK for async in-progress).
    int len = static_cast<int>(addrlen);
    return static_cast<int>(::connect(m_fd, const_cast<sockaddr *>(addr), len));
}

bool socket::in_progress() const {
    // A non-blocking connect() that has not yet completed returns EINPROGRESS
    // on POSIX or WSAEWOULDBLOCK on Windows; either way the connection is
    // establishing asynchronously and the caller should poll for writability.
    return (last_error() == WSAEWOULDBLOCK);
}

} // namespace silicon::network

#endif // defined(SILICON_PLATFORM_WINDOWS)

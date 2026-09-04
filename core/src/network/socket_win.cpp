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

#if defined(_MSC_VER)
import silicon.network;
#endif

import silicon.coroutine;

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::network {

int socket_duplicate_handle(int fd) {

    return fd;
}

bool socket_enable_address_reuse(int ) {

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

    auto [addr, addrlen] = client_endpoint.native_mutable_data();

    int len = static_cast<int>(*addrlen);
    auto raw = ::accept(m_fd, addr, &len);
    *addrlen = static_cast<socklen_t>(len);
    return socket{static_cast<int>(raw)};
}

int socket::last_error() const { return static_cast<int>(WSAGetLastError()); }

int socket::connect(const socket_address &endpoint) {
    auto [addr, addrlen] = endpoint.data();

    int len = static_cast<int>(addrlen);
    return static_cast<int>(::connect(m_fd, const_cast<sockaddr *>(addr), len));
}

bool socket::in_progress() const {

    return (last_error() == WSAEWOULDBLOCK);
}

}

#endif








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

import silicon.coroutine;

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::network {

int socket_duplicate_handle(int fd) { return ::dup(fd); }

bool socket_enable_address_reuse(int fd) {
    int sock_opt{1};

#if defined(SILICON_PLATFORM_LINUX)

    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &sock_opt, static_cast<int>(sizeof(sock_opt))) < 0) {
        return false;
    }
#endif



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


    flags = (block == blocking_t::yes) ? flags & ~O_NONBLOCK : (flags | O_NONBLOCK);

    return (fcntl(m_fd, F_SETFL, flags) == 0);
}

bool socket::shutdown(silicon::scheduler::poll_op how) {
    if(m_fd != -1) {
        int h{0};
        switch(how) {
            case silicon::scheduler::poll_op::read:
                h = SHUT_RD;
                break;
            case silicon::scheduler::poll_op::write:
                h = SHUT_WR;
                break;
            case silicon::scheduler::poll_op::read_write:
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


    auto [addr, addrlen] = client_endpoint.native_mutable_data();

    return socket{::accept(m_fd, addr, addrlen)};
}

int socket::last_error() const { return errno; }

int socket::connect(const socket_address &endpoint) {
    auto [addr, addrlen] = endpoint.data();

    return ::connect(m_fd, const_cast<sockaddr *>(addr), addrlen);
}

bool socket::in_progress() const {



    return (last_error() == EINPROGRESS);
}

}

#endif

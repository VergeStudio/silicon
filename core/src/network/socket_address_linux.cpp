module;

#if defined(SILICON_PLATFORM_LINUX)
#    include <netinet/in.h>
#    include <sys/socket.h>
#endif

module silicon.network;

#if defined(_MSC_VER)
import silicon.network;
#endif

#if defined(SILICON_PLATFORM_LINUX)

namespace silicon::network {

// Linux 平台接缝：sockaddr 无 sin_len/sin6_len 字段，空实现。
// BSD/Apple 真实填充见 socket_address_bsd.cpp，Windows 见 socket_address_win.cpp。

void network_set_sockaddr_len(sockaddr_in *) { }

void network_set_sockaddr_len6(sockaddr_in6 *) { }

}

#endif

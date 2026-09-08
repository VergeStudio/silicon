module;

#if defined(SILICON_PLATFORM_WINDOWS)
#    ifndef WIN32_LEAN_AND_MEAN
#        define WIN32_LEAN_AND_MEAN
#    endif
#    include <winsock2.h>
#    include <ws2tcpip.h>
#endif

module silicon.network;

#if defined(_MSC_VER)
import silicon.network;
#endif

#if defined(SILICON_PLATFORM_WINDOWS)

namespace silicon::network {

// Windows 平台接缝：sockaddr 无 sin_len/sin6_len 字段，空实现。
// BSD/Apple 真实填充见 socket_address_bsd.cpp，Linux 见 socket_address_linux.cpp。

void network_set_sockaddr_len(sockaddr_in *) { }

void network_set_sockaddr_len6(sockaddr_in6 *) { }

}

#endif

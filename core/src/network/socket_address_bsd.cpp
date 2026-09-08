module;

#if defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)
#    include <netinet/in.h>
#    include <sys/socket.h>
#endif

module silicon.network;

#if defined(_MSC_VER)
import silicon.network;
#endif

#if defined(SILICON_PLATFORM_BSD) || defined(SILICON_PLATFORM_APPLE)

namespace silicon::network {

// BSD/macOS 平台接缝：BSD socket 地址需显式填充 sin_len/sin6_len。
// Linux/Windows 无此字段要求，空实现见 socket_address_linux.cpp / _win.cpp。

void network_set_sockaddr_len(sockaddr_in *sin) { sin->sin_len = sizeof(sockaddr_in); }

void network_set_sockaddr_len6(sockaddr_in6 *sin6) { sin6->sin6_len = sizeof(sockaddr_in6); }

}

#endif

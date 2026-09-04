module;

#include <cstdint>
#include <mutex>
#include <coroutine>

module silicon.network;

namespace silicon::network::dns {
uint64_t m_ares_count{0};
std::mutex m_ares_mutex{};
}

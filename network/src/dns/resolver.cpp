// Implementation unit for silicon::network::dns.
//
// Defines the c-ares global init/cleanup counters declared (as extern) in the
// :dns interface partition. The resolver<executor_type> / result<executor_type>
// templates keep their full inline definitions in the partition.

module;

#include <cstdint>
#include <mutex>
#include <coroutine>

module silicon.network;

namespace silicon::network::dns {
uint64_t m_ares_count{0};
std::mutex m_ares_mutex{};
} // namespace silicon::network::dns

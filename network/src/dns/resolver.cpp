#include "silicon/network_impl_includes.hpp"


namespace silicon::network::dns {
uint64_t m_ares_count{0};
std::mutex m_ares_mutex{};
} // namespace silicon::network::dns

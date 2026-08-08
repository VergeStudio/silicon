// Implementation unit for silicon::network (domain_t to_string).
//
// NOTE: ip_address itself is fully inline in the :core partition; only this
// free helper lives here.

module;

#include <stdexcept>
#include <string>

module silicon.network;

namespace silicon::network {

static std::string domain_ipv4{"ipv4"};
static std::string domain_ipv6{"ipv6"};

auto to_string(domain_t domain) -> const std::string & {
    switch(domain) {
        case domain_t::kIpv4:
            return domain_ipv4;
        case domain_t::kIpv6:
            return domain_ipv6;
    }
    throw std::runtime_error{"silicon::network::to_string(domain_t) unknown domain"};
}

} // namespace silicon::network

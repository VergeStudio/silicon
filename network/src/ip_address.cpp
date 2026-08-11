// Implementation unit for silicon::network (domain_t to_string).
//
// NOTE: ip_address itself is fully inline in the :core partition; only this
// free helper lives here.

module;

#include <expected>
#include <string_view>
#include <system_error>

module silicon.network;

import silicon.error;

namespace silicon::network {

auto to_string(domain_t domain) -> result<std::string_view> {
    switch(domain) {
        case domain_t::kIpv4:
            return std::string_view{"ipv4"};
        case domain_t::kIpv6:
            return std::string_view{"ipv6"};
    }
    return std::unexpected(error::make_error_code(error::network_error::kInvalidDomain));
}

} // namespace silicon::network

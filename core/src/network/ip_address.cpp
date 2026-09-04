




module;

#include <expected>
#include <string_view>
#include <system_error>
#include <coroutine>

module silicon.network;

namespace silicon::network {

auto to_string(domain_t domain) -> result<std::string_view> {
    switch(domain) {
        case domain_t::kIpv4:
            return std::string_view{"ipv4"};
        case domain_t::kIpv6:
            return std::string_view{"ipv6"};
    }
    return std::unexpected(make_error_code(network_error::kInvalidDomain));
}

}

// Implementation unit for silicon::network (core connect_status helpers).
//
// Class declarations now live in the :core interface partition (module
// purview); this unit provides the out-of-line, non-template definitions. It
// gets the network types via the implicit import of the primary interface and
// pulls in only the external/standard headers it needs.

module;

#include <expected>
#include <string_view>
#include <system_error>
#include <coroutine>

module silicon.network;

namespace silicon::network {

auto to_string(const connect_status &status) -> result<std::string_view> {
    switch(status) {
        case connect_status::kConnected:
            return std::string_view{"connected"};
        case connect_status::kInvalidIpAddress:
            return std::string_view{"invalid_ip_address"};
        case connect_status::kTimeout:
            return std::string_view{"timeout"};
        case connect_status::kError:
            return std::string_view{"error"};
    }

    return std::unexpected(make_error_code(network_error::kInvalidConnectStatus));
}

} // namespace silicon::network

module;

#include <expected>
#include <string_view>
#include <system_error>
#include <coroutine>

module silicon.network;

namespace silicon::network {

auto to_string(const connect_status &status) -> silicon::error::result<std::string_view> {
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

}

#include "silicon/network_impl_includes.hpp"


namespace silicon::network {
const static std::string connect_status_connected{"connected"};
const static std::string connect_status_invalid_ip_address{"invalid_ip_address"};
const static std::string connect_status_timeout{"timeout"};
const static std::string connect_status_error{"error"};

auto to_string(const connect_status &status) -> const std::string & {
    switch(status) {
        case connect_status::kConnected:
            return connect_status_connected;
        case connect_status::kInvalidIpAddress:
            return connect_status_invalid_ip_address;
        case connect_status::kTimeout:
            return connect_status_timeout;
        case connect_status::kError:
            return connect_status_error;
    }

    throw std::logic_error{"Invalid/unknown connect status."};
}

} // namespace silicon::network

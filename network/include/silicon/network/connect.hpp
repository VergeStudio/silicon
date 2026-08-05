#pragma once

#include <string>

namespace silicon::network {
enum class connect_status {
    /// The connection has been established.
    kConnected,
    /// The given ip address could not be parsed or is invalid.
    kInvalidIpAddress,
    /// The connection operation timed out.
    kTimeout,
    /// There was an error, use errno to get more information on the specific error.
    kError
};

/**
 * @param status String representation of the connection status.
 * @throw std::logic_error If provided an invalid connect_status enum value.
 */
auto to_string(const connect_status &status) -> const std::string &;

} // namespace silicon::network

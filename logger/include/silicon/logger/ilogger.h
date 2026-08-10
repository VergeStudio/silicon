#pragma once

#include <cstdint>
#include <source_location>
#include <string_view>

namespace silicon::logger {

enum class log_level : std::uint8_t {
    kTrace,
    kDebug,
    kInfo,
    kWarn,
    kError,
    kCritical,
    kOff,
};

/// @brief Abstract interface for a logger.
///
/// The concrete default_logger implements this interface.
class logger {
  public:
    logger() = default;
    logger(const logger &) = delete;
    logger(logger &&) = delete;
    auto operator=(const logger &) -> logger & = delete;
    auto operator=(logger &&) -> logger & = delete;
    virtual ~logger() = default;

    virtual void trace(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;
    virtual void debug(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;
    virtual void info(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;
    virtual void warning(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;
    virtual void error(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;
    virtual void critical(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;

    virtual void set_log_level(log_level level) const = 0;
};

} // namespace silicon::logger

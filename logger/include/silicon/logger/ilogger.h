#pragma once

#include <cstdint>
#include <source_location>
#include <string_view>

namespace silicon::logger {

enum class LogLevel : std::uint8_t {
    Trace,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off,
};

/// @brief Abstract interface for a logger.
///
/// The concrete DefaultLogger implements this interface.
class ILogger {
  public:
    ILogger() = default;
    ILogger(const ILogger &) = delete;
    ILogger(ILogger &&) = delete;
    auto operator=(const ILogger &) -> ILogger & = delete;
    auto operator=(ILogger &&) -> ILogger & = delete;
    virtual ~ILogger() = default;

    virtual void Trace(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;
    virtual void Debug(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;
    virtual void Info(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;
    virtual void Warning(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;
    virtual void Error(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;
    virtual void Critical(const std::string_view &msg, std::source_location &&loc = std::source_location::current()) const = 0;

    virtual void SetLogLevel(LogLevel level) const = 0;
};

} // namespace silicon::logger

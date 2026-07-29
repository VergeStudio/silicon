#ifndef SILICON_LOGGER_H
#define SILICON_LOGGER_H

#include <cstdint>
#include <source_location>
#include <string_view>

#include "silicon/logger/common.h"
#include "silicon/logger/ilogger.h"

namespace silicon::logger {
// LogLevel 与 ILogger 统一在 ilogger.h 中定义，避免重复声明导致底层类型冲突。

LOGGER_API void Init(const std::string_view &, LogLevel, int32_t, int32_t, int32_t);

LOGGER_API void Stop();

LOGGER_API void SetLogLevel(LogLevel);

LOGGER_API void Trace(const std::string_view &, std::source_location &&rLocation = std::source_location::current());
LOGGER_API void Debug(const std::string_view &, std::source_location &&rLocation = std::source_location::current());
LOGGER_API void Info(const std::string_view &, std::source_location &&rLocation = std::source_location::current());
LOGGER_API void Warning(const std::string_view &, std::source_location &&rLocation = std::source_location::current());
LOGGER_API void Error(const std::string_view &, std::source_location &&rLocation = std::source_location::current());
LOGGER_API void Critical(const std::string_view &, std::source_location &&rLocation = std::source_location::current());

} // namespace silicon::logger

#endif // SILICON_LOGGER_H

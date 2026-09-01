#ifndef SILICON_LOGGER_H
#define SILICON_LOGGER_H

#include <cstdint>
#include <source_location>
#include <string_view>

#include "silicon/common.h"
#include "silicon/logger/ilogger.h"

namespace silicon::logger {
// log_level 与 logger 统一在 ilogger.h 中定义，避免重复声明导致底层类型冲突。

LOGGER_API void init(const std::string_view &, log_level, int32_t, int32_t, int32_t);

LOGGER_API void stop();

LOGGER_API void set_log_level(log_level);

LOGGER_API void trace(const std::string_view &, std::source_location &&location = std::source_location::current());
LOGGER_API void debug(const std::string_view &, std::source_location &&location = std::source_location::current());
LOGGER_API void info(const std::string_view &, std::source_location &&location = std::source_location::current());
LOGGER_API void warning(const std::string_view &, std::source_location &&location = std::source_location::current());
LOGGER_API void error(const std::string_view &, std::source_location &&location = std::source_location::current());
LOGGER_API void critical(const std::string_view &, std::source_location &&location = std::source_location::current());

} // namespace silicon::logger

#endif // SILICON_LOGGER_H

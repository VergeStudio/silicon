#pragma once

#include <cstdint>

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

/// @brief logger 接口的类型擦除门面定义于 logger.cppm（logger_facade）。
/// 具体实现（default_logger）以鸭子类型满足门面，无需继承。

} // namespace silicon::logger

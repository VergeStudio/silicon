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

/// @brief logger 接口已由 silicon.proxy 门面承载（见 logger.cppm 的 logger_facade）。
/// 具体实现（default_logger）以鸭子类型满足门面，无需继承抽象基类。

} // namespace silicon::logger

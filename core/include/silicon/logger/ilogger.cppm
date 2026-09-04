module;

#include <cstdint>

export module silicon.logger:ilogger;

export namespace silicon::logger {

enum class log_level : std::uint8_t {
    kTrace,
    kDebug,
    kInfo,
    kWarn,
    kError,
    kCritical,
    kOff,
};




}

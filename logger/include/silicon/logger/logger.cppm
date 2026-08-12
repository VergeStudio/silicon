module;

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <source_location>
#include <string_view>
#include <system_error>

#include "silicon/logger/common.h"
#include "silicon/logger/ilogger.h"

export module silicon.logger;

export import :config;

export namespace silicon::logger {

using ::silicon::logger::log_level;

/// logger 模块专属错误码枚举。
enum class logger_error {
    kInitFailed = 1,
    kInvalidLevel,
};

/// 返回 logger_error 专属 error_category（name() = "silicon.logger"）。
[[nodiscard]] inline const std::error_category &logger_category() noexcept {
    static const class : public std::error_category {
        const char *name() const noexcept override { return "silicon.logger"; }
        std::string message(int ev) const override {
            switch(static_cast<logger_error>(ev)) {
                case logger_error::kInitFailed: return "logger init failed";
                case logger_error::kInvalidLevel: return "invalid log level";
            }
            return "unknown logger error";
        }
    } cat;
    return cat;
}

/// 将 logger_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(logger_error e) noexcept {
    return {static_cast<int>(e), logger_category()};
}

void init(const std::string_view &, log_level, int32_t, int32_t, int32_t);
void stop();
void set_log_level(log_level);
void trace(const std::string_view &, std::source_location &&location = std::source_location::current());
void debug(const std::string_view &, std::source_location &&location = std::source_location::current());
void info(const std::string_view &, std::source_location &&location = std::source_location::current());
void warning(const std::string_view &, std::source_location &&location = std::source_location::current());
void error(const std::string_view &, std::source_location &&location = std::source_location::current());
void critical(const std::string_view &, std::source_location &&location = std::source_location::current());

} // namespace silicon::logger

// default_logger 的单一定义源位于头文件 default_logger.h（继承 logger、虚方法 override），
// 模块仅 re-export 该头文件，避免与实现单元产生 ODR 双定义。
export {
#include "silicon/logger/default_logger.h"
}

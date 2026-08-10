module;

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <source_location>
#include <string_view>

#include "silicon/logger/common.h"
#include "silicon/logger/ilogger.h"

export module silicon.logger;

export import :config;

export namespace silicon::logger {

using ::silicon::logger::log_level;

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

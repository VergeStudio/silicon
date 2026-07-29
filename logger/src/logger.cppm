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

using ::silicon::logger::LogLevel;

void Init(const std::string_view &, LogLevel, int32_t, int32_t, int32_t);
void Stop();
void SetLogLevel(LogLevel);
void Trace(const std::string_view &, std::source_location &&rLocation = std::source_location::current());
void Debug(const std::string_view &, std::source_location &&rLocation = std::source_location::current());
void Info(const std::string_view &, std::source_location &&rLocation = std::source_location::current());
void Warning(const std::string_view &, std::source_location &&rLocation = std::source_location::current());
void Error(const std::string_view &, std::source_location &&rLocation = std::source_location::current());
void Critical(const std::string_view &, std::source_location &&rLocation = std::source_location::current());

} // namespace silicon::logger

// DefaultLogger 的单一定义源位于头文件 default_logger.h（继承 ILogger、虚方法 override），
// 模块仅 re-export 该头文件，避免与实现单元产生 ODR 双定义。
export {
#include "silicon/logger/default_logger.h"
}

module;

// MSVC C++20 模块：<ctime> 等标准头在全局模块片段被包含后，其 using 声明
// （如 std::mktime）被模块重导出时触发 C5304（声明具有内部链接），并被
// 视作错误（C2220）。该警告系工具链已知误报，禁用整个模块重导出警告族。
#pragma warning(disable : 5301 5302 5303 5304 5305)

#include <ctime>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "spdlog/async.h"
#include "spdlog/async_logger.h"
#include "spdlog/common.h"
#include "spdlog/details/registry.h"
#include "spdlog/logger.h"
#include "spdlog/sinks/hourly_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

export module spdlog;

namespace spdlog {
export using spdlog::shutdown;
export using spdlog::logger;
export using spdlog::sink_ptr;
export using spdlog::sinks_init_list;
export using spdlog::async_logger;
export using spdlog::thread_pool;
export using spdlog::init_thread_pool;
export using spdlog::spdlog_ex;
export using spdlog::hourly_logger_mt;

namespace sinks {
export using spdlog::sinks::stdout_color_sink_mt;
export using spdlog::sinks::rotating_file_sink_mt;
export using spdlog::sinks::hourly_file_sink_mt;
} // namespace sinks

namespace details {
export using spdlog::details::registry;
} // namespace details

namespace level {
export using spdlog::level::level_enum;
} // namespace level
} // namespace spdlog

// module spdlog;
// module;

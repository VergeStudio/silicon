module;

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

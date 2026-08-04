#include "silicon/logger/logger.h"

#include "silicon/logger/default_logger.h"

namespace {
silicon::logger::DefaultLogger g_logger;
}

namespace silicon::logger {
void Init(const std::string_view &log_path, LogLevel log_level, int32_t queue_size, int32_t thread_num, int32_t backtrace_num) {
    return g_logger.Init(log_path, log_level, queue_size, thread_num, backtrace_num);
}

void Stop() {
    return g_logger.Stop();
}

void SetLogLevel(LogLevel log_level) {
    return g_logger.SetLogLevel(log_level);
}

void Trace(const std::string_view &msg, std::source_location &&location) {
    return g_logger.Trace(msg, std::forward<std::source_location>(location));
}

void Debug(const std::string_view &msg, std::source_location &&location) {
    return g_logger.Debug(msg, std::forward<std::source_location>(location));
}

void Info(const std::string_view &msg, std::source_location &&location) {
    return g_logger.Info(msg, std::forward<std::source_location>(location));
}

void Warning(const std::string_view &msg, std::source_location &&location) {
    return g_logger.Warning(msg, std::forward<std::source_location>(location));
}

void Error(const std::string_view &msg, std::source_location &&location) {
    return g_logger.Error(msg, std::forward<std::source_location>(location));
}

void Critical(const std::string_view &msg, std::source_location &&location) {
    return g_logger.Critical(msg, std::forward<std::source_location>(location));
}
} // namespace silicon::logger

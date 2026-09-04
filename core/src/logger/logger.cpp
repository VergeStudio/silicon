module;

#include <expected>
#include <source_location>
#include <string_view>
#include <system_error>
#include <utility>

module silicon.logger;

namespace {
silicon::logger::default_logger g_logger;
}

namespace silicon::logger {
std::expected<void, std::error_code> init(const std::string_view &log_path, log_level log_level, int32_t queue_size, int32_t thread_num, int32_t backtrace_num) {
    return g_logger.init(log_path, log_level, queue_size, thread_num, backtrace_num);
}

void stop() {
    return g_logger.stop();
}

void set_log_level(log_level log_level) {
    return g_logger.set_log_level(log_level);
}

void trace(const std::string_view &msg, std::source_location &&location) {
    return g_logger.trace(msg, std::forward<std::source_location>(location));
}

void debug(const std::string_view &msg, std::source_location &&location) {
    return g_logger.debug(msg, std::forward<std::source_location>(location));
}

void info(const std::string_view &msg, std::source_location &&location) {
    return g_logger.info(msg, std::forward<std::source_location>(location));
}

void warning(const std::string_view &msg, std::source_location &&location) {
    return g_logger.warning(msg, std::forward<std::source_location>(location));
}

void error(const std::string_view &msg, std::source_location &&location) {
    return g_logger.error(msg, std::forward<std::source_location>(location));
}

void critical(const std::string_view &msg, std::source_location &&location) {
    return g_logger.critical(msg, std::forward<std::source_location>(location));
}
}

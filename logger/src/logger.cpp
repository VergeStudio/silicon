#include "silicon/logger/logger.h"

#include "silicon/logger/default_logger.h"

namespace {
silicon::logger::DefaultLogger g_logger;
}

namespace silicon::logger {
void Init(const std::string_view &rLogPath, LogLevel logLevel, int32_t qsize, int32_t threadNum, int32_t backtraceNum) {
    return g_logger.Init(rLogPath, logLevel, qsize, threadNum, backtraceNum);
}

void Stop() {
    return g_logger.Stop();
}

void SetLogLevel(LogLevel logLevel) {
    return g_logger.SetLogLevel(logLevel);
}

void Trace(const std::string_view &msg, std::source_location &&rLocation) {
    return g_logger.Trace(msg, std::forward<std::source_location>(rLocation));
}

void Debug(const std::string_view &msg, std::source_location &&rLocation) {
    return g_logger.Debug(msg, std::forward<std::source_location>(rLocation));
}

void Info(const std::string_view &msg, std::source_location &&rLocation) {
    return g_logger.Info(msg, std::forward<std::source_location>(rLocation));
}

void Warning(const std::string_view &msg, std::source_location &&rLocation) {
    return g_logger.Warning(msg, std::forward<std::source_location>(rLocation));
}

void Error(const std::string_view &msg, std::source_location &&rLocation) {
    return g_logger.Error(msg, std::forward<std::source_location>(rLocation));
}

void Critical(const std::string_view &msg, std::source_location &&rLocation) {
    return g_logger.Critical(msg, std::forward<std::source_location>(rLocation));
}
} // namespace silicon::logger

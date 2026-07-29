#ifndef SILICON_LOGGER_MANAGER_H
#define SILICON_LOGGER_MANAGER_H

#include <source_location>
#include <string_view>
#include <memory>
#include <mutex>
#include <atomic>

#include "silicon/logger/logger.h"

namespace silicon::logger {

class GlobalLogger {
 public:
    GlobalLogger();
    ~GlobalLogger() noexcept;

 public:
    void Init(const std::string_view &, LogLevel, int32_t, int32_t, int32_t);
    void CreateLogger(LogLevel, const std::string_view &, int32_t);
    void SetLogLevel(LogLevel) const;
    void Stop();

 public:
    void Trace(const std::string_view &, std::source_location &&) const;
    void Debug(const std::string_view &, std::source_location &&) const;
    void Info(const std::string_view &, std::source_location &&) const;
    void Warning(const std::string_view &, std::source_location &&) const;
    void Error(const std::string_view &, std::source_location &&) const;
    void Critical(const std::string_view &, std::source_location &&) const;

 private:
    // Pimpl: hides spdlog types from the public interface
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::mutex m_mutex;
    std::atomic<bool> m_isInitialized{false};
};

} // namespace silicon::logger

#endif // SILICON_LOGGER_MANAGER_H

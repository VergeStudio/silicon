#ifndef SILICON_LOGGER_GLOBAL_LOGGER_H
#define SILICON_LOGGER_GLOBAL_LOGGER_H

#include <source_location>
#include <string_view>
#include <memory>
#include <mutex>
#include <atomic>

#include "silicon/logger/logger.h"

namespace silicon::logger {

class global_logger {
 public:
    global_logger();
    ~global_logger() noexcept;

 public:
    void Init(const std::string_view &, log_level, int32_t, int32_t, int32_t);
    void CreateLogger(log_level, const std::string_view &, int32_t);
    void SetLogLevel(log_level) const;
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
    std::unique_ptr<Impl> impl_;
    std::mutex mutex_;
    std::atomic<bool> is_initialized_{false};
};

} // namespace silicon::logger

#endif // SILICON_LOGGER_GLOBAL_LOGGER_H

#ifndef SILICON_LOGGER_DEFAULT_LOGGER_H
#define SILICON_LOGGER_DEFAULT_LOGGER_H

#include <atomic>
#include <memory>
#include <mutex>
#include <source_location>
#include <string_view>
#include <expected>
#include <system_error>

#include "silicon/logger/ilogger.h"
#include "silicon/common.h"

namespace silicon::logger {

// 单一定义源：与模块接口 silicon.logger 导出的类保持一致（鸭子类型满足 logger_facade，
// 无需继承抽象基类），避免 module 声明与 header 声明产生 ODR 双定义。
class CORE_API default_logger {
  public:
    default_logger();
    ~default_logger() noexcept;

  public:
    [[nodiscard]] std::expected<void, std::error_code> init(const std::string_view &, log_level, int32_t, int32_t, int32_t);
    void create_logger(log_level, const std::string_view &, int32_t);
    void set_log_level(log_level) const;
    void stop();

  public:
    void trace(const std::string_view &, std::source_location &&) const;
    void debug(const std::string_view &, std::source_location &&) const;
    void info(const std::string_view &, std::source_location &&) const;
    void warning(const std::string_view &, std::source_location &&) const;
    void error(const std::string_view &, std::source_location &&) const;
    void critical(const std::string_view &, std::source_location &&) const;

  private:
    // Pimpl: hides spdlog types from the public interface
    struct impl;
    std::unique_ptr<impl> impl_;
    std::mutex mutex_;
    std::atomic<bool> is_initialized_{false};
};

} // namespace silicon::logger

#endif // SILICON_LOGGER_DEFAULT_LOGGER_H

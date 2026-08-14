#ifndef SILICON_LOGGER_DEFAULT_LOGGER_H
#define SILICON_LOGGER_DEFAULT_LOGGER_H

#include <atomic>
#include <memory>
#include <mutex>
#include <source_location>
#include <string_view>

#include "silicon/logger/ilogger.h"

namespace silicon::logger {

// 单一定义源：与模块接口 silicon.logger 导出的类保持一致（继承 logger、虚方法 override），
// 避免 module 声明与 header 声明产生 ODR 双定义、导致跨 DLL 虚函数修饰名不匹配。
class default_logger: public logger {
  public:
    default_logger();
    ~default_logger() noexcept override;

  public:
    void init(const std::string_view &, log_level, int32_t, int32_t, int32_t);
    void create_logger(log_level, const std::string_view &, int32_t);
    void set_log_level(log_level) const override;
    void stop();

  public:
    void trace(const std::string_view &, std::source_location &&) const override;
    void debug(const std::string_view &, std::source_location &&) const override;
    void info(const std::string_view &, std::source_location &&) const override;
    void warning(const std::string_view &, std::source_location &&) const override;
    void error(const std::string_view &, std::source_location &&) const override;
    void critical(const std::string_view &, std::source_location &&) const override;

  private:
    // Pimpl: hides spdlog types from the public interface
    struct impl;
    std::unique_ptr<impl> impl_;
    std::mutex mutex_;
    std::atomic<bool> is_initialized_{false};
};

} // namespace silicon::logger

#endif // SILICON_LOGGER_DEFAULT_LOGGER_H

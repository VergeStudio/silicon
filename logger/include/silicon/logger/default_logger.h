#ifndef SILICON_LOGGER_DEFAULT_LOGGER_H
#define SILICON_LOGGER_DEFAULT_LOGGER_H

#include <atomic>
#include <memory>
#include <mutex>
#include <source_location>
#include <string_view>

#include "silicon/logger/ilogger.h"

namespace silicon::logger {

// 单一定义源：与模块接口 silicon.logger 导出的类保持一致（继承 Logger、虚方法 override），
// 避免 module 声明与 header 声明产生 ODR 双定义、导致跨 DLL 虚函数修饰名不匹配。
class DefaultLogger: public Logger {
  public:
    DefaultLogger();
    ~DefaultLogger() noexcept override;

  public:
    void Init(const std::string_view &, LogLevel, int32_t, int32_t, int32_t);
    void CreateLogger(LogLevel, const std::string_view &, int32_t);
    void SetLogLevel(LogLevel) const override;
    void Stop();

  public:
    void Trace(const std::string_view &, std::source_location &&) const override;
    void Debug(const std::string_view &, std::source_location &&) const override;
    void Info(const std::string_view &, std::source_location &&) const override;
    void Warning(const std::string_view &, std::source_location &&) const override;
    void Error(const std::string_view &, std::source_location &&) const override;
    void Critical(const std::string_view &, std::source_location &&) const override;

  private:
    // Pimpl: hides spdlog types from the public interface
    struct Impl;
    std::unique_ptr<Impl> impl_;
    std::mutex mutex_;
    std::atomic<bool> is_initialized_{false};
};

} // namespace silicon::logger

#endif // SILICON_LOGGER_DEFAULT_LOGGER_H

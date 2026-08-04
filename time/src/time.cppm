module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

export module silicon.time;

export namespace silicon::time {

/// 时钟抽象（可注入，便于测试）
class Clock {
  public:
    virtual ~Clock() = default;
    virtual std::chrono::system_clock::time_point now() const = 0;
    virtual std::int64_t now_ms() const = 0;
};

/// 默认系统时钟（包装 std::chrono::system_clock）
class SystemClock : public Clock {
  public:
    std::chrono::system_clock::time_point now() const override;
    std::int64_t now_ms() const override;
};

/// 日期源抽象：提供当前日期字符串（供 System Context 的 DefaultDateSource 使用）
class DateSource {
  public:
    virtual ~DateSource() = default;
    virtual std::string current_date() const = 0;
};

/// 默认日期实现（基于 Clock，返回 UTC 日期 YYYY-MM-DD）
class DefaultDateSource : public DateSource {
    struct Impl {
      public:
        const Clock* clock_{nullptr};
    };
    std::unique_ptr<Impl> impl_{std::make_unique<Impl>()};

  public:
    explicit DefaultDateSource(const Clock& clock) { impl_->clock_ = &clock; }
    std::string current_date() const override;
};

} // namespace silicon::time

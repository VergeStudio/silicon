module;

#include <chrono>
#include <cstdint>
#include <string>

export module silicon.time;

export namespace silicon::time {

/// 时钟抽象（可注入，便于测试）
class IClock {
  public:
    virtual ~IClock() = default;
    virtual std::chrono::system_clock::time_point now() const = 0;
    virtual std::int64_t now_ms() const = 0;
};

/// 默认系统时钟（包装 std::chrono::system_clock）
class SystemClock: public IClock {
  public:
    std::chrono::system_clock::time_point now() const override;
    std::int64_t now_ms() const override;
};

/// 日期源抽象：提供当前日期字符串（供 System Context 的 DateSource 使用）
class IDateSource {
  public:
    virtual ~IDateSource() = default;
    virtual std::string current_date() const = 0;
};

/// 默认日期实现（基于 IClock，返回 UTC 日期 YYYY-MM-DD）
class DateSource: public IDateSource {
    const IClock &clock_;

  public:
    explicit DateSource(const IClock &clock): clock_(clock) {}
    std::string current_date() const override;
};

} // namespace silicon::time

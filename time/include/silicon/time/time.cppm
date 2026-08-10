module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

export module silicon.time;

export namespace silicon::time {

/// 时钟抽象（可注入，便于测试）
class i_clock {
  public:
    virtual ~i_clock() = default;
    virtual std::chrono::system_clock::time_point now() const = 0;
    virtual std::int64_t now_ms() const = 0;
};

/// 默认系统时钟（包装 std::chrono::system_clock）
class system_clock : public i_clock {
  public:
    std::chrono::system_clock::time_point now() const override;
    std::int64_t now_ms() const override;
};

/// 日期源抽象：提供当前日期字符串（供 System Context 的 date_source 使用）
class i_date_source {
  public:
    virtual ~i_date_source() = default;
    virtual std::string current_date() const = 0;
};

/// 默认日期实现（基于 i_clock，返回 UTC 日期 YYYY-MM-DD）
class date_source : public i_date_source {
    struct Impl {
      public:
        const i_clock* clock_{nullptr};
    };
    std::unique_ptr<Impl> impl_{std::make_unique<Impl>()};

  public:
    explicit date_source(const i_clock& clock) { impl_->clock_ = &clock; }
    std::string current_date() const override;
};

} // namespace silicon::time

module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include <tuple>
// proxy dispatch 宏头：宏不随 C++20 模块导出，必须在全局模块片段文本包含
#include <silicon/proxy/proxy_macros.h>
export module silicon.time;


import silicon.proxy;

export namespace silicon::time {

/// 时钟门面（type-erased，鸭子类型满足即可）
PRO_DEF_MEM_DISPATCH(MemClockNow, now);
PRO_DEF_MEM_DISPATCH(MemClockNowMs, now_ms);
struct clock_facade : silicon::proxy::facade_builder
    ::add_convention<MemClockNow,
                     std::chrono::system_clock::time_point() const>
    ::add_convention<MemClockNowMs, std::int64_t() const>::build {};

using clock_proxy = silicon::proxy::proxy<clock_facade>;
using clock_view = silicon::proxy::proxy_view<clock_facade>;

template <class T, class... Args>
[[nodiscard]] clock_proxy make_clock(Args &&...args) {
    return silicon::proxy::make_proxy<clock_facade, T>(
        std::forward<Args>(args)...);
}

/// 为已存在的时钟对象创建非拥有视图；调用方负责保证生命周期。
template <class T>
    requires silicon::proxy::proxiable_target<T, clock_facade>
[[nodiscard]] clock_view make_clock_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<clock_facade>(target);
}

/// 默认系统时钟（包装 std::chrono::system_clock）
class system_clock {
  public:
    std::chrono::system_clock::time_point now() const;
    std::int64_t now_ms() const;
};

/// 日期源门面：提供当前日期字符串
PRO_DEF_MEM_DISPATCH(MemDateSourceCurrentDate, current_date);
struct date_source_facade : silicon::proxy::facade_builder
    ::add_convention<MemDateSourceCurrentDate, std::string() const>::build {};

using date_source_proxy = silicon::proxy::proxy<date_source_facade>;
using date_source_view = silicon::proxy::proxy_view<date_source_facade>;

template <class T, class... Args>
[[nodiscard]] date_source_proxy make_date_source(Args &&...args) {
    return silicon::proxy::make_proxy<date_source_facade, T>(
        std::forward<Args>(args)...);
}

/// 默认日期实现（基于 clock 门面，返回 UTC 日期 YYYY-MM-DD）
class date_source {
    struct impl {
      public:
        clock_view clock_;
    };
    std::unique_ptr<impl> impl_{std::make_unique<impl>()};

  public:
    explicit date_source(const clock_view &clock) { impl_->clock_ = clock; }
    std::string current_date() const;
};

} // namespace silicon::time

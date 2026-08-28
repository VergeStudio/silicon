module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include <tuple>
// proxy dispatch 宏头：宏不随 C++20 模块导出，必须在全局模块片段文本包含
#include <silicon/core/proxy/proxy_macros.h>
// 单 DLL 伞宏（TIME_API）：同全局模块片段文本包含，不随模块导出
#include <silicon/time/common.h>
export module silicon.time;


import silicon.proxy;

export namespace silicon::time {

/// 时钟门面（type-erased，鸭子类型满足即可）
/// 注意：约定用 add_direct_convention（直接约定），这样 proxy<clock_facade>
/// 会生成 now()/now_ms() 成员，可由 clock_proxy 直接以成员形式调用。
/// （add_convention 是 indirect 约定，只能经 invoke 调用，而 vendored proxy 的
///  invoke 在间接约定下对 owning/observer proxy 均有 meta 基类不匹配的 bug，
///  且间接约定也不会暴露为代理成员，与 date_source / SystemContext 的
///  `clock.now_ms()` / `clock_->now()` 成员式用法不一致。）
PRO_DEF_MEM_DISPATCH(MemClockNow, now);
PRO_DEF_MEM_DISPATCH(MemClockNowMs, now_ms);
struct clock_facade : silicon::proxy::facade_builder
    ::add_direct_convention<MemClockNow,
                            std::chrono::system_clock::time_point() const>
    ::add_direct_convention<MemClockNowMs, std::int64_t() const>::build {};

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
class TIME_API system_clock {
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
/// 持有 owning clock_proxy（直接约定生成的 now() 成员仅在 owning proxy 上可用，
/// observer_facade 会丢弃直接约定，故这里用 clock_proxy 而非 clock_view）。
class TIME_API date_source {
    struct impl {
      public:
        clock_proxy clock_;
    };
    std::unique_ptr<impl> impl_{std::make_unique<impl>()};

  public:
    explicit date_source(clock_proxy clock) { impl_->clock_ = std::move(clock); }
    std::string current_date() const;
};

} // namespace silicon::time

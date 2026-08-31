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
// system_clock 为 header-only（global module 实体）：跨工具链 mangling 兼容
// （MSVC 命名模块符号带 ::<!module> 标签而 clang 不带），见该头内注释
#include <silicon/time/system_clock.h>
export module silicon.time;


import silicon.proxy;

export namespace silicon::time {

/// 时钟门面（type-erased，鸭子类型满足即可）
/// 约定用 add_convention（即 add_indirect_convention，间接约定），与
/// silicon.ai.llm 的 provider_facade 等保持一致。间接约定的成员不暴露为
/// proxy 的直属成员，而是经 proxy::operator->() 返回的间接访问器基类调用
/// （如 clock_->now_ms() / clock_->now()），observer 视图同样可经 operator->
/// 访问。直接约定（add_direct_convention）在此不成立：proxy 把目标存为
/// compact_ptr/inplace_ptr 指针包装，间接约定的 proxiable 检查解引用后的
/// 目标类型（system_clock 拥有 now/now_ms），而直接约定检查存储包装类型本身
/// （无 now/now_ms 成员），会触发 consteval C3615。
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

/// 默认系统时钟：定义于 <silicon/time/system_clock.h>（header-only，详见该头
/// 内的跨工具链 mangling 说明）。此处 re-export 供模块消费方按名使用；由于 IFC
/// 不含成员函数体，需要完整类型（make_clock<system_clock>、直接构造等）的
/// 消费方须自行 #include <silicon/time/system_clock.h>。
using silicon::time::system_clock;

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
/// 持有 owning clock_proxy；间接约定成员经 clock_->now() 访问（owning 与
/// observer 视图均可经 operator-> 取到，故用 clock_proxy 而非 clock_view 仅因
/// 此处需要所有权语义，与约定形态无关）。
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

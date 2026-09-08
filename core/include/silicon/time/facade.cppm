module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include <tuple>

#include <silicon/proxy/proxy_macros.h>

#include <silicon/common.h>

export module silicon.time;

export import silicon.time.system_clock;
export import silicon.time.steady_clock;
export import silicon.time.sleep;
export import silicon.time.timing_wheel;

import silicon.proxy;

export namespace silicon::time {

PRO_DEF_MEM_DISPATCH(MemClockNow, now);
PRO_DEF_MEM_DISPATCH(MemClockNowMs, now_ms);
struct clock_facade : silicon::proxy::facade_builder
    ::add_convention<MemClockNow,
                     std::chrono::system_clock::time_point() const>
    ::add_convention<MemClockNowMs, std::int64_t() const>::build {};


template <class T, class... Args>
[[nodiscard]] silicon::proxy::proxy<clock_facade> make_clock(Args &&...args) {
    return silicon::proxy::make_proxy<clock_facade, T>(
        std::forward<Args>(args)...);
}

template <class T>
    requires silicon::proxy::proxiable_target<T, clock_facade>
[[nodiscard]] silicon::proxy::proxy_view<clock_facade> make_clock_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<clock_facade>(target);
}

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

class SILICON_CORE_API date_source {
  private:
    struct impl {
      public:
        silicon::proxy::proxy<clock_facade> clock_;
    };
    std::unique_ptr<impl> impl_{std::make_unique<impl>()};

  public:
    explicit date_source(silicon::proxy::proxy<clock_facade> clock) { impl_->clock_ = std::move(clock); }
    std::string current_date() const;
};

}

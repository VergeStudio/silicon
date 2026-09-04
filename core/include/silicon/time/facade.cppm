module;

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include <tuple>

#include <silicon/proxy/proxy_macros.h>

#include <silicon/common.h>


#include <silicon/time/system_clock.h>
export module silicon.time;


import silicon.proxy;

export namespace silicon::time {










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


template <class T>
    requires silicon::proxy::proxiable_target<T, clock_facade>
[[nodiscard]] clock_view make_clock_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<clock_facade>(target);
}





using silicon::time::system_clock;


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





class CORE_API date_source {
    struct impl {
      public:
        clock_proxy clock_;
    };
    std::unique_ptr<impl> impl_{std::make_unique<impl>()};

  public:
    explicit date_source(clock_proxy clock) { impl_->clock_ = std::move(clock); }
    std::string current_date() const;
};

}

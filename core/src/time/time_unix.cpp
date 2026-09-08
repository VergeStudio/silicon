module;

#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>

#include <silicon/common.h>

module silicon.time;

#if defined(_MSC_VER)
import silicon.time;
#endif

#if defined(SILICON_PLATFORM_UNIX)

namespace silicon::time {

std::string date_source::current_date() const {
    auto tp = impl_->clock_->now();
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm gmt{};
    ::gmtime_r(&tt, &gmt);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", gmt.tm_year + 1900, gmt.tm_mon + 1, gmt.tm_mday);
    return std::string(buf);
}

}

#endif

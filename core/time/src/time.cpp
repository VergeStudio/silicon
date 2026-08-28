module;

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <memory>
#include <string>

module silicon.time;

namespace silicon::time {

std::chrono::system_clock::time_point system_clock::now() const {
    return std::chrono::system_clock::now();
}

std::int64_t system_clock::now_ms() const {
    using namespace std::chrono;
    return duration_cast<milliseconds>(now().time_since_epoch()).count();
}

std::string date_source::current_date() const {
    auto tp = impl_->clock_.now();
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm gmt{};
#if defined(SILICON_PLATFORM_WINDOWS)
    gmtime_s(&gmt, &tt);
#else
    gmtime_r(&tt, &gmt);
#endif
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d", gmt.tm_year + 1900, gmt.tm_mon + 1, gmt.tm_mday);
    return std::string(buf);
}

} // namespace silicon::time

module;

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <source_location>
#include <string_view>
#include <system_error>

#include "silicon/common.h"
#include "silicon/logger/ilogger.h"
#include <tuple>
#include <silicon/proxy/proxy_macros.h>

export module silicon.logger;

export import silicon.logger.error;
import silicon.proxy;

export namespace silicon::logger {

using ::silicon::logger::log_level;

// logger 接口：类型擦除门面
PRO_DEF_MEM_DISPATCH(MemLoggerTrace, trace);
PRO_DEF_MEM_DISPATCH(MemLoggerDebug, debug);
PRO_DEF_MEM_DISPATCH(MemLoggerInfo, info);
PRO_DEF_MEM_DISPATCH(MemLoggerWarning, warning);
PRO_DEF_MEM_DISPATCH(MemLoggerError, error);
PRO_DEF_MEM_DISPATCH(MemLoggerCritical, critical);
PRO_DEF_MEM_DISPATCH(MemLoggerSetLevel, set_log_level);

struct logger_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemLoggerTrace, void(const std::string_view &, std::source_location &&) const>
      ::add_convention<MemLoggerDebug, void(const std::string_view &, std::source_location &&) const>
      ::add_convention<MemLoggerInfo, void(const std::string_view &, std::source_location &&) const>
      ::add_convention<MemLoggerWarning, void(const std::string_view &, std::source_location &&) const>
      ::add_convention<MemLoggerError, void(const std::string_view &, std::source_location &&) const>
      ::add_convention<MemLoggerCritical, void(const std::string_view &, std::source_location &&) const>
      ::add_convention<MemLoggerSetLevel, void(log_level) const>
      ::build {};

using logger_proxy = silicon::proxy::proxy<logger_facade>;
using logger_view = silicon::proxy::proxy_view<logger_facade>;

template<class T, class... Args>
[[nodiscard]] logger_proxy make_logger(Args &&...args) {
    return silicon::proxy::make_proxy<logger_facade, T>(std::forward<Args>(args)...);
}

template<class T>
requires silicon::proxy::proxiable_target<T, logger_facade>
[[nodiscard]] logger_view make_logger_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<logger_facade>(target);
}

void init(const std::string_view &, log_level, int32_t, int32_t, int32_t);
void stop();
void set_log_level(log_level);
void trace(const std::string_view &, std::source_location &&location = std::source_location::current());
void debug(const std::string_view &, std::source_location &&location = std::source_location::current());
void info(const std::string_view &, std::source_location &&location = std::source_location::current());
void warning(const std::string_view &, std::source_location &&location = std::source_location::current());
void error(const std::string_view &, std::source_location &&location = std::source_location::current());
void critical(const std::string_view &, std::source_location &&location = std::source_location::current());

} // namespace silicon::logger

// default_logger 的单一定义源位于头文件 default_logger.h（鸭子类型满足 logger_facade，
// 无需继承抽象基类），模块仅 re-export 该头文件，避免与实现单元产生 ODR 双定义。
export {
#include "silicon/logger/default_logger.h"
}

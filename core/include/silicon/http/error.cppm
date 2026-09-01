module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>
export module silicon.http.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::http {

CORE_API std::atomic<const std::error_category *> http_error_category_instance{nullptr};

} // namespace silicon::http

export namespace silicon::http {

/// http 模块专属错误码枚举。
enum class http_error {
    kRequestFailed = 1,
    kInvalidResponse,
    kTimeout,
    kUnknown,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
class CORE_API http_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.http"; }
    std::string message(int ev) const override {
        switch(static_cast<http_error>(ev)) {
            case http_error::kRequestFailed: return "http request failed";
            case http_error::kInvalidResponse: return "invalid http response";
            case http_error::kTimeout: return "http request timed out";
            case http_error::kUnknown: return "unknown http error";
        }
        return "unknown http error";
    }
};

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_http_error_category(const std::error_category &cat) noexcept {
    http_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 http_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &http_category() noexcept {
    const std::error_category *cat = http_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// 将 http_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(http_error e) noexcept {
    return {static_cast<int>(e), http_category()};
}

} // namespace silicon::http

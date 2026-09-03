module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>
export module silicon.tui.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::tui {

CORE_API std::atomic<const std::error_category *> tui_error_category_instance{nullptr};

} // namespace silicon::tui

export namespace silicon::tui {

/// tui 模块专属错误码枚举。
enum class tui_error {
    kInitFailed = 1,
    kInvalidTerminal,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
class CORE_API tui_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.tui"; }
    std::string message(int ev) const override {
        switch(static_cast<tui_error>(ev)) {
            case tui_error::kInitFailed: return "tui init failed";
            case tui_error::kInvalidTerminal: return "invalid terminal";
        }
        return "unknown tui error";
    }
};

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_tui_error_category(const std::error_category &cat) noexcept {
    tui_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 tui_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &tui_category() noexcept {
    const std::error_category *cat = tui_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// 将 tui_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(tui_error e) noexcept {
    return {static_cast<int>(e), tui_category()};
}

} // namespace silicon::tui

// category 自注册：与 silicon.network / config / logger / event / library / di /
// scheduler 一致，避免未注入消费方走错误路径时 tui_category() 直接 std::terminate。
// 匿名命名空间须置于模块作用域（export 块之外），否则 clang 报
// "anonymous namespaces cannot be exported"。组合根仍可调 inject_tui_error_category
// 注入自定义实例（原子存储，后写覆盖）。
namespace {
    const silicon::tui::tui_category_impl s_default_tui_category{};
    const bool s_tui_category_registered = [] {
        silicon::tui::inject_tui_error_category(s_default_tui_category);
        return true;
    }();
} // namespace

module;

#include <exception>
#include <memory>
#include <string>
#include <system_error>

export module silicon.tui.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::tui {

inline std::unique_ptr<const std::error_category, silicon::error::category_deleter> tui_error_category_instance;

} // namespace silicon::tui

export namespace silicon::tui {

/// tui 模块专属错误码枚举。
enum class tui_error {
    kInitFailed = 1,
    kInvalidTerminal,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
class tui_category_impl final : public std::error_category {
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
    tui_error_category_instance.reset(&cat);
}

/// 返回 tui_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &tui_category() noexcept {
    if (!tui_error_category_instance) {
        std::terminate();
    }
    return *tui_error_category_instance;
}

/// 将 tui_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(tui_error e) noexcept {
    return {static_cast<int>(e), tui_category()};
}

} // namespace silicon::tui

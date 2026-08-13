module;

#include <string>
#include <system_error>

export module silicon.tui.error;

export namespace silicon::tui {

/// tui 模块专属错误码枚举。
enum class tui_error {
    kInitFailed = 1,
    kInvalidTerminal,
};

/// 返回 tui_error 专属 error_category（name() = "silicon.tui"）。
[[nodiscard]] inline const std::error_category &tui_category() noexcept {
    static const class : public std::error_category {
        const char *name() const noexcept override { return "silicon.tui"; }
        std::string message(int ev) const override {
            switch(static_cast<tui_error>(ev)) {
                case tui_error::kInitFailed: return "tui init failed";
                case tui_error::kInvalidTerminal: return "invalid terminal";
            }
            return "unknown tui error";
        }
    } cat;
    return cat;
}

/// 将 tui_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(tui_error e) noexcept {
    return {static_cast<int>(e), tui_category()};
}

} // namespace silicon::tui

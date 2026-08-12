module;

#include <memory>
#include <string>
#include <string_view>
#include <system_error>

import silicon.core;

export module silicon.tui;

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

// ── 终端抽象 ─────────────────────────────────────────────────────

class i_terminal {
  public:
    virtual ~i_terminal() = default;
    virtual std::string_view terminal_type() const = 0;
    virtual int32_t width() const = 0;
    virtual int32_t height() const = 0;
};

#if defined(SILICON_PLATFORM_UNIX)

class unix_terminal: public i_terminal {
  public:
    std::string_view terminal_type() const override;
    int32_t width() const override;
    int32_t height() const override;
};
#else
class default_terminal: public i_terminal {
  public:
    std::string_view terminal_type() const override;
    int32_t width() const override;
    int32_t height() const override;
};
#endif

// ── PTY 抽象 ─────────────────────────────────────────────────────

class i_pty {
  public:
    virtual ~i_pty() = default;
    virtual bool create(std::string_view working_dir, std::initializer_list<std::string> env) = 0;
    virtual int32_t write(std::string_view data) = 0;
    virtual std::string read() = 0;
    virtual void close() = 0;
};

// ── TUI 渲染器 ──────────────────────────────────────────────────

class i_tui_renderer {
  public:
    virtual ~i_tui_renderer() = default;
    virtual void render(std::string_view text) = 0;
    virtual void clear() = 0;
};

} // namespace silicon::tui

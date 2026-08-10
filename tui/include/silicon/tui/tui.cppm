module;

#include <memory>
#include <string>
#include <string_view>

import silicon.core;

export module silicon.tui;

export namespace silicon::tui {

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

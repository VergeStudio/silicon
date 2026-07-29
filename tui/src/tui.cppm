module;

#include <memory>
#include <string>
#include <string_view>

import silicon.core;

export module silicon.tui;

export namespace silicon::tui {

// ── 终端抽象 ─────────────────────────────────────────────────────

class ITerminal {
  public:
    virtual ~ITerminal() = default;
    virtual std::string_view terminal_type() const = 0;
    virtual int32_t width() const = 0;
    virtual int32_t height() const = 0;
};

#if defined(SILICON_PLATFORM_UNIX)

class UnixTerminal: public ITerminal {
  public:
    std::string_view terminal_type() const override;
    int32_t width() const override;
    int32_t height() const override;
};
#else
class DefaultTerminal: public ITerminal {
  public:
    std::string_view terminal_type() const override;
    int32_t width() const override;
    int32_t height() const override;
};
#endif

// ── PTY 抽象 ─────────────────────────────────────────────────────

class IPty {
  public:
    virtual ~IPty() = default;
    virtual bool create(std::string_view working_dir, std::initializer_list<std::string> env) = 0;
    virtual int32_t write(std::string_view data) = 0;
    virtual std::string read() = 0;
    virtual void close() = 0;
};

// ── TUI 渲染器 ──────────────────────────────────────────────────

class ITuiRenderer {
  public:
    virtual ~ITuiRenderer() = default;
    virtual void render(std::string_view text) = 0;
    virtual void clear() = 0;
};

} // namespace silicon::tui

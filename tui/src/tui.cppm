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
    virtual std::string_view TerminalType() const = 0;
    virtual int32_t Width() const = 0;
    virtual int32_t Height() const = 0;
};

#if defined(SILICON_PLATFORM_UNIX)

class UnixTerminal: public ITerminal {
  public:
    std::string_view TerminalType() const override;
    int32_t Width() const override;
    int32_t Height() const override;
};
#else
class DefaultTerminal: public ITerminal {
  public:
    std::string_view TerminalType() const override;
    int32_t Width() const override;
    int32_t Height() const override;
};
#endif

// ── PTY 抽象 ─────────────────────────────────────────────────────

class IPty {
  public:
    virtual ~IPty() = default;
    virtual bool Create(std::string_view working_dir, std::initializer_list<std::string> env) = 0;
    virtual int32_t Write(std::string_view data) = 0;
    virtual std::string Read() = 0;
    virtual void Close() = 0;
};

// ── TUI 渲染器 ──────────────────────────────────────────────────

class ITuiRenderer {
  public:
    virtual ~ITuiRenderer() = default;
    virtual void Render(std::string_view text) = 0;
    virtual void Clear() = 0;
};

} // namespace silicon::tui

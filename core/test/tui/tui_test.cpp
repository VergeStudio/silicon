// tui 模块测试：覆盖基于 silicon.proxy 的三类 type-erased 门面
// （terminal / pty / tui_renderer）的 make_proxy / make_proxy_view 分派，
// 以及 tui 错误码经 silicon.tui category 的构造。
#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>

#include <silicon/test/test.h>

import silicon.proxy;
import silicon.tui;

namespace tui = silicon::tui;

namespace {

// 具体终端：满足 terminal_facade 的 terminal_type/width/height 三个约定。
struct fake_terminal {
    std::string_view terminal_type() const { return "xterm-256color"; }
    int32_t width() const { return 80; }
    int32_t height() const { return 24; }
};

// 具体 PTY：满足 pty_facade 的 create/write/read/close 四个约定。
struct fake_pty {
    bool create(std::string_view, std::initializer_list<std::string>) { return true; }
    int32_t write(std::string_view s) { return static_cast<int32_t>(s.size()); }
    std::string read() { return "out"; }
    void close() {}
};

// 具体渲染器：满足 tui_renderer_facade 的 render/clear 两个约定。
struct fake_renderer {
    void render(std::string_view) {}
    void clear() {}
};

} // namespace

TEST_CASE("tui 错误码走 silicon.tui category") {
    const std::error_code ec = tui::make_error_code(tui::tui_error::kInitFailed);
    CHECK(ec.value() == static_cast<int>(tui::tui_error::kInitFailed));
    CHECK(std::string(ec.category().name()) == "silicon.tui");
    CHECK(std::string(ec.message()) == "tui init failed");

    const std::error_code invalid = tui::make_error_code(tui::tui_error::kInvalidTerminal);
    CHECK(std::string(invalid.message()) == "invalid terminal");
    CHECK(invalid != ec);
}

TEST_CASE("terminal 门面：make_terminal 分派到目标实现") {
    auto term = tui::make_terminal<fake_terminal>();
    REQUIRE(term.has_value());
    CHECK(std::string(term->terminal_type()) == "xterm-256color");
    CHECK(term->width() == 80);
    CHECK(term->height() == 24);
}

TEST_CASE("terminal 门面：make_terminal_view 以非拥有视图擦除") {
    fake_terminal ft;
    auto view = tui::make_terminal_view(ft);
    REQUIRE(view.has_value());
    CHECK(std::string(view->terminal_type()) == "xterm-256color");
    CHECK(view->width() == 80);
}

TEST_CASE("pty 门面：make_pty 分派到目标实现") {
    auto pty = tui::make_pty<fake_pty>();
    REQUIRE(pty.has_value());
    CHECK(pty->create("sh", {"-l"}) == true);
    CHECK(pty->write("hi") == 2);
    CHECK(pty->read() == "out");
    pty->close();
}

TEST_CASE("pty 门面：make_pty_view 以非拥有视图擦除") {
    fake_pty fp;
    auto view = tui::make_pty_view(fp);
    REQUIRE(view.has_value());
    CHECK(view->write("abc") == 3);
}

TEST_CASE("tui_renderer 门面：make_tui_renderer 分派到目标实现") {
    auto renderer = tui::make_tui_renderer<fake_renderer>();
    REQUIRE(renderer.has_value());
    renderer->render("hello");
    renderer->clear();
    CHECK(true);
}

TEST_CASE("tui_renderer 门面：make_tui_renderer_view 以非拥有视图擦除") {
    fake_renderer fr;
    auto view = tui::make_tui_renderer_view(fr);
    REQUIRE(view.has_value());
    view->render("content");
    view->clear();
    CHECK(true);
}

#include <memory>
#include <silicon/test/test.h>
#include <string>

import silicon.platform;

using namespace silicon::platform;

TEST_CASE("create_platform 返回非空的当前平台实现") {
    auto p = create_platform();
    REQUIRE(p != nullptr);

    bool sep_ok = (p->path_separator() == '\\') || (p->path_separator() == '/');
    CHECK(sep_ok);
}

TEST_CASE("create_platform 行为符合当前 OS") {
    auto p = create_platform();
    REQUIRE(p != nullptr);
#if defined(SILICON_PLATFORM_WINDOWS)
    CHECK(p->os_name() == "windows");
    CHECK(p->path_separator() == '\\');
    CHECK(p->line_ending() == "\r\n");
#else
    bool name_ok = (p->os_name() == "linux") || (p->os_name() == "unix");
    CHECK(name_ok);
    CHECK(p->path_separator() == '/');
    CHECK(p->line_ending() == "\n");
#endif
}

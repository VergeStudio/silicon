#include <memory>
#include <silicon/test/test.hpp>
#include <string>

import silicon.platform;

using namespace silicon::platform;

TEST_CASE("create_platform 返回非空的当前平台实现") {
    auto p = create_platform();
    REQUIRE(p != nullptr);
    // 路径分隔符与换行符因平台而异，仅做形态校验。
    bool sep_ok = (p->PathSeparator() == '\\') || (p->PathSeparator() == '/');
    CHECK(sep_ok);
}

TEST_CASE("create_platform 行为符合当前 OS") {
    auto p = create_platform();
    REQUIRE(p != nullptr);
#if defined(SILICON_PLATFORM_WINDOWS)
    CHECK(p->OsName() == "windows");
    CHECK(p->PathSeparator() == '\\');
    CHECK(p->LineEnding() == "\r\n");
#else
    bool name_ok = (p->OsName() == "linux") || (p->OsName() == "unix");
    CHECK(name_ok);
    CHECK(p->PathSeparator() == '/');
    CHECK(p->LineEnding() == "\n");
#endif
}

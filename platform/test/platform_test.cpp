#include <memory>
#include <silicon/test/test.hpp>
#include <string>

import silicon.platform;

using namespace silicon::platform;

TEST_CASE("CreatePlatform 返回非空的当前平台实现") {
    auto p = CreatePlatform();
    REQUIRE(p != nullptr);
    // 路径分隔符与换行符因平台而异，仅做形态校验。
    bool sep_ok = (p->PathSeparator() == '\\') || (p->PathSeparator() == '/');
    CHECK(sep_ok);
}

TEST_CASE("CreatePlatform 行为符合当前 OS") {
    auto p = CreatePlatform();
    REQUIRE(p != nullptr);
#if defined(_WIN32)
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

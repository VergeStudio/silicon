#include <cstdlib>
#include <silicon/test/test.h>
#include <string>

import silicon.xdg;

using namespace silicon::xdg;

TEST_CASE("xdg base dirs are non-empty on current platform") {
    CHECK_FALSE(home_dir().empty());
    CHECK_FALSE(config_home().empty());
    CHECK_FALSE(data_home().empty());
    CHECK_FALSE(cache_home().empty());
    CHECK_FALSE(state_home().empty());
}

TEST_CASE("xdg dir collections follow platform defaults") {
    auto cd = config_dirs();
    auto dd = data_dirs();
#if defined(SILICON_PLATFORM_WINDOWS)
    // Windows 无系统级 XDG 目录概念，按设计返回空集合。
    CHECK(cd.empty());
    CHECK(dd.empty());
#else
    // POSIX 默认包含 /etc/xdg 与 /usr/local/share:/usr/share。
    CHECK(cd.size() >= 1);
    CHECK(dd.size() >= 1);
#endif
}

#if !defined(SILICON_PLATFORM_WINDOWS)
TEST_CASE("xdg honors XDG_CONFIG_HOME override") {
    const char *prev = std::getenv("XDG_CONFIG_HOME");
    std::string backup = prev ? std::string(prev) : "";
    setenv("XDG_CONFIG_HOME", "/tmp/xdgtest", 1);
    CHECK(config_home() == "/tmp/xdgtest");
    if(backup.empty())
        unsetenv("XDG_CONFIG_HOME");
    else
        setenv("XDG_CONFIG_HOME", backup.c_str(), 1);
}
#endif

#include <silicon/test/test.hpp>
#include <string>

import silicon.cli;

using namespace silicon::cli;

TEST_CASE("CliParser: 无参数返回空结果") {
    Parser p;
    const char *argv[] = {"prog"};
    auto r = p.Parse(1, argv);
    CHECK(r.command().empty());
    CHECK(r.flags().empty());
    CHECK(r.positional().empty());
}

TEST_CASE("CliParser: 标志解析") {
    Parser p;
    const char *argv[] = {"prog", "--name", "silicon", "-v", "pos1"};
    auto r = p.Parse(5, argv);
    CHECK(r.command().empty());
    CHECK(r.flags().at("name") == std::string("silicon"));
    CHECK(r.flags().at("v") == std::string("true"));
    CHECK(r.positional().size() == 1);
    CHECK(r.positional()[0] == std::string("pos1"));
}

TEST_CASE("CliParser: 子命令+标志+位置参数") {
    Parser p;
    const char *argv[] = {"prog", "run", "--config", "dev.json", "arg1", "arg2"};
    auto r = p.Parse(6, argv);
    CHECK(r.command() == std::string("run"));
    CHECK(r.flags().at("config") == std::string("dev.json"));
    CHECK(r.positional().size() == 2);
}

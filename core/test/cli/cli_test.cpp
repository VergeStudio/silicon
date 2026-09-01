#include <memory>
#include <silicon/test/test.hpp>
#include <string>
#include <vector>

// parser / parse_result / cli_error / make_error_code 已折为 header-only 全局实体
// （见 parser_types.h / cli_error_defs.h），须直接文本包含，不能经 import silicon.cli 取得。
#include <silicon/cli/parser/parser_types.h>
#include <silicon/cli/error/cli_error_defs.h>

import silicon.cli;

using namespace silicon::cli;

TEST_CASE("en_empty_result") {
    parser p;
    const char *argv[] = {"prog"};
    auto r = p.parse(1, argv);
    CHECK(r.has_value());
    CHECK(r->command().empty());
    CHECK(r->flags().empty());
    CHECK(r->positional().empty());
}

TEST_CASE("en_flag_parse") {
    parser p;
    const char *argv[] = {"prog", "--name", "silicon", "-v", "pos1"};
    auto r = p.parse(5, argv);
    CHECK(r.has_value());
    CHECK(r->command().empty());
    CHECK(r->flags().at("name") == std::string("silicon"));
    CHECK(r->flags().at("v") == std::string("true"));
    CHECK(r->positional().size() == 1);
    CHECK(r->positional()[0] == std::string("pos1"));
}

TEST_CASE("en_subcmd_flags_positional") {
    parser p;
    const char *argv[] = {"prog", "run", "--config", "dev.json", "arg1", "arg2"};
    auto r = p.parse(6, argv);
    CHECK(r.has_value());
    CHECK(r->command() == std::string("run"));
    CHECK(r->flags().at("config") == std::string("dev.json"));
    CHECK(r->positional().size() == 2);
}

TEST_CASE("en_unknown_subcommand") {
    parser p;
    p.add_subcommand("run");
    p.add_subcommand("build");
    const char *argv[] = {"prog", "frobnicate"};
    auto r = p.parse(2, argv);
    CHECK_FALSE(r.has_value());
    CHECK(r.error() == make_error_code(cli_error::kUnknownSubcommand));
}

TEST_CASE("en_malformed_flag") {
    parser p;
    const char *argv1[] = {"prog", "-"};
    auto r1 = p.parse(2, argv1);
    CHECK_FALSE(r1.has_value());
    CHECK(r1.error() == make_error_code(cli_error::kInvalidValue));

    // 裸 '--' 为分隔符（GNU 惯例）：其后全部为位置参数。
    const char *argv2[] = {"prog", "--", "--weird", "pos"};
    auto r2 = p.parse(4, argv2);
    REQUIRE(r2.has_value());
    CHECK(r2->positional().size() == 2);
    CHECK(r2->positional()[0] == std::string("--weird"));
    CHECK(r2->positional()[1] == std::string("pos"));
}

TEST_CASE("en_long_flag_equals_value") {
    parser p;
    p.add_flag("name");
    p.add_flag("verbose");
    const char *argv[] = {"prog", "--name=dev.json", "--verbose=1"};
    auto r = p.parse(3, argv);
    REQUIRE(r.has_value());
    CHECK(r->flags().at("name") == std::string("dev.json"));
    CHECK(r->flags().at("verbose") == std::string("1"));
}

TEST_CASE("en_registered_value_flag_consumes_negative") {
    parser p;
    p.add_flag("num", true);
    const char *argv[] = {"prog", "--num", "-5"};
    auto r = p.parse(3, argv);
    REQUIRE(r.has_value());
    CHECK(r->flags().at("num") == std::string("-5"));
}

TEST_CASE("en_registered_bool_flag_not_consume_next") {
    parser p;
    p.add_flag("verbose"); // requires_value=false：声明驱动，不吞后随 token
    const char *argv[] = {"prog", "--verbose", "dev.json"};
    auto r = p.parse(3, argv);
    REQUIRE(r.has_value());
    CHECK(r->flags().at("verbose") == std::string("true"));
    CHECK(r->positional().size() == 1);
    CHECK(r->positional()[0] == std::string("dev.json"));
}

TEST_CASE("en_unknown_flag") {
    parser p;
    p.add_flag("name");
    const char *argv[] = {"prog", "--verbose"};
    auto r = p.parse(2, argv);
    CHECK_FALSE(r.has_value());
    CHECK(r.error() == make_error_code(cli_error::kUnknownOption));
}

TEST_CASE("en_missing_value") {
    parser p;
    p.add_flag("name", true);
    const char *argv[] = {"prog", "--name"};
    auto r = p.parse(2, argv);
    CHECK_FALSE(r.has_value());
    CHECK(r.error() == make_error_code(cli_error::kMissingArgument));
}

TEST_CASE("en_registered_ok") {
    parser p;
    p.add_subcommand("run");
    p.add_flag("name", true);
    p.add_flag("verbose");
    const char *argv[] = {"prog", "run", "--name", "dev.json", "--verbose"};
    auto r = p.parse(5, argv);
    CHECK(r.has_value());
    CHECK(r->command() == std::string("run"));
    CHECK(r->flags().at("name") == std::string("dev.json"));
    CHECK(r->flags().at("verbose") == std::string("true"));
}

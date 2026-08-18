#include <memory>
#include <silicon/test/test.hpp>
#include <string>
#include <vector>

import silicon.cli;

using namespace silicon::cli;

TEST_CASE("en_empty_result") {
    Parser p;
    const char *argv[] = {"prog"};
    auto r = p.Parse(1, argv);
    CHECK(r.has_value());
    CHECK(r->command().empty());
    CHECK(r->flags().empty());
    CHECK(r->positional().empty());
}

TEST_CASE("en_flag_parse") {
    Parser p;
    const char *argv[] = {"prog", "--name", "silicon", "-v", "pos1"};
    auto r = p.Parse(5, argv);
    CHECK(r.has_value());
    CHECK(r->command().empty());
    CHECK(r->flags().at("name") == std::string("silicon"));
    CHECK(r->flags().at("v") == std::string("true"));
    CHECK(r->positional().size() == 1);
    CHECK(r->positional()[0] == std::string("pos1"));
}

TEST_CASE("en_subcmd_flags_positional") {
    Parser p;
    const char *argv[] = {"prog", "run", "--config", "dev.json", "arg1", "arg2"};
    auto r = p.Parse(6, argv);
    CHECK(r.has_value());
    CHECK(r->command() == std::string("run"));
    CHECK(r->flags().at("config") == std::string("dev.json"));
    CHECK(r->positional().size() == 2);
}

TEST_CASE("en_unknown_subcommand") {
    Parser p;
    p.AddSubcommand("run");
    p.AddSubcommand("build");
    const char *argv[] = {"prog", "frobnicate"};
    auto r = p.Parse(2, argv);
    CHECK_FALSE(r.has_value());
    CHECK(r.error() == make_error_code(cli_error::kUnknownSubcommand));
}

TEST_CASE("en_malformed_flag") {
    Parser p;
    const char *argv1[] = {"prog", "-"};
    auto r1 = p.Parse(2, argv1);
    CHECK_FALSE(r1.has_value());
    CHECK(r1.error() == make_error_code(cli_error::kInvalidValue));

    const char *argv2[] = {"prog", "--"};
    auto r2 = p.Parse(2, argv2);
    CHECK_FALSE(r2.has_value());
    CHECK(r2.error() == make_error_code(cli_error::kInvalidValue));
}

TEST_CASE("en_unknown_flag") {
    Parser p;
    p.AddFlag("name");
    const char *argv[] = {"prog", "--verbose"};
    auto r = p.Parse(2, argv);
    CHECK_FALSE(r.has_value());
    CHECK(r.error() == make_error_code(cli_error::kUnknownOption));
}

TEST_CASE("en_missing_value") {
    Parser p;
    p.AddFlag("name", true);
    const char *argv[] = {"prog", "--name"};
    auto r = p.Parse(2, argv);
    CHECK_FALSE(r.has_value());
    CHECK(r.error() == make_error_code(cli_error::kMissingArgument));
}

TEST_CASE("en_registered_ok") {
    Parser p;
    p.AddSubcommand("run");
    p.AddFlag("name", true);
    p.AddFlag("verbose");
    const char *argv[] = {"prog", "run", "--name", "dev.json", "--verbose"};
    auto r = p.Parse(5, argv);
    CHECK(r.has_value());
    CHECK(r->command() == std::string("run"));
    CHECK(r->flags().at("name") == std::string("dev.json"));
    CHECK(r->flags().at("verbose") == std::string("true"));
}

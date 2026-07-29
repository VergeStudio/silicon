module;

#include <string>
#include <vector>

#include "silicon/cli/common.h"

export module silicon.cli;

export import :config;

export namespace silicon::cli {

struct CLI_API ParseResult {
    std::string command;
    std::vector<std::string> args;
    bool empty() const noexcept { return command.empty(); }
};

class CLI_API Parser {
  public:
    Parser() = default;
    ~Parser() = default;

    auto Parse(int argc, const char *const *argv) -> ParseResult;
};

} // namespace silicon::cli

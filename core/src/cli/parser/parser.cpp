module;

#include <expected>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

module silicon.cli.parser;

import silicon.cli.error;
import silicon.cli.parser.parse_result;

namespace silicon::cli {

struct parser::impl {
    std::set<std::string> subcommands;
    std::map<std::string, bool> flags;
};

parser::parser() : impl_(std::make_unique<impl>()) {}

parser::~parser() = default;

void parser::add_subcommand(std::string name) {
    impl_->subcommands.insert(std::move(name));
}

void parser::add_flag(std::string name, bool requires_value) {
    impl_->flags.emplace(std::move(name), requires_value);
}

std::expected<parse_result, std::error_code> parser::parse(int argc, const char *const *argv) const {
    parse_result result;
    if(argc <= 0) return result;

    int i = 1;

    if(i < argc && argv[i][0] != '-') {
        result.command() = argv[i];
        ++i;
    }

    struct flag_seen {
        std::string name;
        bool has_value;
    };
    std::vector<flag_seen> seen;

    bool positional_only = false;
    while(i < argc) {
        std::string_view arg(argv[i]);
        if(positional_only || arg.empty() || arg[0] != '-') {
            result.positional().emplace_back(argv[i]);
            ++i;
            continue;
        }

        const bool is_long = arg.size() >= 2 && arg[1] == '-';

        if(is_long && arg.size() == 2) {
            positional_only = true;
            ++i;
            continue;
        }

        if(!is_long && arg.size() == 1) {
            return std::unexpected(make_error_code(cli_error::kInvalidValue));
        }

        const size_t name_start = is_long ? 2 : 1;
        std::string name(arg.substr(name_start));

        std::optional<std::string> inline_value;
        if(is_long) {
            if(auto eq = name.find('='); eq != std::string::npos) {
                inline_value = name.substr(eq + 1);
                name.erase(eq);
            }
        }
        if(name.empty()) {
            return std::unexpected(make_error_code(cli_error::kInvalidValue));
        }

        const auto it = impl_->flags.find(name);
        bool takes_value;
        if(it != impl_->flags.end()) {
            takes_value = it->second;
        } else {
            takes_value = is_long && !inline_value.has_value() && (i + 1 < argc) && (argv[i + 1][0] != '-');
        }

        if(inline_value.has_value()) {
            result.flags()[name] = std::move(*inline_value);
            seen.push_back({std::move(name), true});
            ++i;
        } else if(takes_value) {
            if(i + 1 >= argc) {
                return std::unexpected(make_error_code(cli_error::kMissingArgument));
            }
            result.flags()[name] = argv[i + 1];
            seen.push_back({std::move(name), true});
            i += 2;
        } else {
            result.flags()[name] = "true";
            seen.push_back({std::move(name), false});
            ++i;
        }
    }

    if(!impl_->subcommands.empty() && !result.command().empty()
       && impl_->subcommands.find(result.command()) == impl_->subcommands.end()) {
        return std::unexpected(make_error_code(cli_error::kUnknownSubcommand));
    }
    if(!impl_->flags.empty()) {
        for(const auto &f: seen) {
            auto it = impl_->flags.find(f.name);
            if(it == impl_->flags.end()) {
                return std::unexpected(make_error_code(cli_error::kUnknownOption));
            }
            if(it->second && !f.has_value) {
                return std::unexpected(make_error_code(cli_error::kMissingArgument));
            }
        }
    }

    return result;
}

}

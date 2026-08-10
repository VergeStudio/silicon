module;

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

module silicon.cli.parser;

namespace silicon::cli {

parse_result Parser::Parse(int argc, const char *const *argv) const {
    parse_result result;
    if(argc <= 0) return result;
    int i = 1;
    if(i < argc && argv[i][0] != '-') {
        result.Command() = argv[i];
        ++i;
    }
    while(i < argc) {
        std::string_view arg(argv[i]);
        if(arg.size() >= 2 && arg[0] == '-') {
            size_t name_start = (arg.size() > 2 && arg[1] == '-') ? 2 : 1;
            bool is_long = (arg.size() > 2 && arg[1] == '-');
            auto name = std::string(arg.substr(name_start));
            if(is_long && i + 1 < argc && argv[i + 1][0] != '-') {
                result.Flags()[std::move(name)] = argv[i + 1];
                i += 2;
            } else {
                result.Flags()[std::move(name)] = "true";
                ++i;
            }
        } else {
            result.Positional().emplace_back(argv[i]);
            ++i;
        }
    }
    return result;
}

} // namespace silicon::cli

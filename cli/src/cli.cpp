module silicon.cli;

namespace silicon::cli {

auto Parser::Parse(int argc, const char *const *argv) -> ParseResult {
    ParseResult result;
    if(argc < 2) return result;

    result.command = argv[1];
    for(int i = 2; i < argc; ++i) {
        result.args.emplace_back(argv[i]);
    }
    return result;
}

} // namespace silicon::cli

module;

#include <expected>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include <tuple>

#include <silicon/proxy/proxy_macros.h>
#include <silicon/common.h>
export module silicon.cli.parser;

export import silicon.cli.parser.parse_result;

import silicon.proxy;

export namespace silicon::cli {

class CORE_API parser {
  public:
    parser();
    ~parser();

    void add_subcommand(std::string);

    void add_flag(std::string, bool = false);

    std::expected<parse_result, std::error_code> parse(int, const char *const *) const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

PRO_DEF_MEM_DISPATCH(MemParserParse, parse);

struct parser_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemParserParse, std::expected<parse_result, std::error_code>(int, const char *const *) const>
      ::build {};

using parser_proxy = silicon::proxy::proxy<parser_facade>;

using parser_view = silicon::proxy::proxy_view<parser_facade>;

template<class T, class... Args>
[[nodiscard]] parser_proxy make_parser(Args &&...args) {
    return silicon::proxy::make_proxy<parser_facade, T>(std::forward<Args>(args)...);
}

}

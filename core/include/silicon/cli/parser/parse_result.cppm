module;

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <silicon/common.h>

export module silicon.cli.parser.parse_result;

export namespace silicon::cli {

class SILICON_CORE_API parse_result {
  public:
    parse_result();
    ~parse_result();

    parse_result(const parse_result &o);
    parse_result &operator=(const parse_result &o);
    parse_result(parse_result &&) noexcept;
    parse_result &operator=(parse_result &&) noexcept;

  public:
    std::string &command();
    const std::string &command() const;
    std::map<std::string, std::string> &flags();
    const std::map<std::string, std::string> &flags() const;
    std::vector<std::string> &positional();
    const std::vector<std::string> &positional() const;

  private:

    struct impl;
    std::shared_ptr<impl> impl_;
};

}

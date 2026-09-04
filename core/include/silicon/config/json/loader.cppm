module;

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <expected>

export module silicon.config.json;

#include "silicon/common.h"

import silicon.json;
import silicon.fs;

export import silicon.config.error;
export import silicon.config.config_value;

export namespace silicon::config {


class CORE_API json_file_config {
  public:
    json_file_config();
    ~json_file_config();
    result<void> load(const std::string &, const fs::file_system_view &);

    std::optional<config_value> get(std::string_view) const;
    std::map<std::string, config_value, std::less<>> all() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

}

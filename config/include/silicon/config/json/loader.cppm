module;

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <expected>

export module silicon.config.json;

#include "silicon/config/common.h"

import silicon.json;
import silicon.fs;

export import silicon.config.error;
export import silicon.config.config_value;

export namespace silicon::config {
/// 基于 JSON 文件的配置加载器（平坦 key=value 映射）。
/// 满足 silicon.fs 的 file_system_facade（鸭子类型），从给定文件系统句柄读取。
CONFIG_API class JsonFileConfig {
  public:
    JsonFileConfig();
    ~JsonFileConfig();
    result<void> Load(const std::string &, const fs::file_system_view &);

    std::optional<config_value> Get(std::string_view) const;
    std::map<std::string, config_value, std::less<>> All() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace silicon::config

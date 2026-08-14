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

export import silicon.config.loader.interface;
export import silicon.config.config_value;

export namespace silicon::config {
/// 基于 JSON 文件的配置加载器（平坦 key=value 映射）
CONFIG_API class JsonFileConfig: public ILoader {
  public:
    JsonFileConfig();
    ~JsonFileConfig();
    result<void> Load(const std::string &path, const fs::IFileSystem &fs);

    // 注意：基类 ILoader 仅声明 virtual Load()；get/all 并非虚函数覆写。
    std::optional<ConfigValue> Get(std::string_view key) const;
    std::map<std::string, ConfigValue, std::less<>> All() const;

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

} // namespace silicon::config

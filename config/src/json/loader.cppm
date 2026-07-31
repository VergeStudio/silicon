module;

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

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
    bool load(const std::string &path, const fs::IFileSystem &fs);

    // 注意：基类 ILoader 仅声明 virtual load()；get/all 并非虚函数覆写。
    std::optional<ConfigValue> get(std::string_view key) const;
    std::map<std::string, ConfigValue, std::less<>> all() const;

  private:
    struct P;
    std::unique_ptr<P> m_p;
};

} // namespace silicon::config

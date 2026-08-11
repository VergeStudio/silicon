module;

#include <expected>
#include <memory>
#include <string>

#include "silicon/config/common.h"

export module silicon.config.loader.interface;

import silicon.error;

export namespace silicon::config {

/// 统一错误返回类型：config 模块可失败 API 返回 config::result<T>。
template<typename T>
using result = std::expected<T, std::error_code>;

class CONFIG_API i_loader {
  public:
    virtual ~i_loader() = default;
    [[nodiscard]] virtual auto Load(const std::string &path) -> result<void> = 0;
};

} // namespace silicon::config

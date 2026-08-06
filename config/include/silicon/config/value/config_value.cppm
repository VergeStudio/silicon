module;

#include <cstdint>
#include <expected>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "silicon/config/common.h"

export module silicon.config.config_value;

export namespace silicon::config {

// Forward declare for recursive variant
class ConfigValue;

using ConfigValueData = std::variant<
    std::nullptr_t,
    bool,
    int64_t,
    double,
    std::string,
    std::shared_ptr<std::vector<ConfigValue>>,
    std::shared_ptr<std::map<std::string, ConfigValue>>>;

class CONFIG_API ConfigValue {
    struct Impl {
        ConfigValueData data_{nullptr};
    };
    std::shared_ptr<Impl> impl_{std::make_shared<Impl>()};

  public:
    ConfigValue() = default;
    ~ConfigValue() = default;

    ConfigValue(std::nullptr_t);
    ConfigValue(bool v);
    ConfigValue(int64_t v);
    ConfigValue(double v);
    ConfigValue(std::string v);

    // 值类型语义：拷贝做深拷贝，不与源对象共享实现
    ConfigValue(const ConfigValue &o): impl_(std::make_shared<Impl>(*o.impl_)) {}
    auto operator=(const ConfigValue &o) -> ConfigValue & {
        if(this != &o) { impl_ = std::make_shared<Impl>(*o.impl_); }
        return *this;
    }
    ConfigValue(ConfigValue &&) noexcept = default;
    auto operator=(ConfigValue &&) noexcept -> ConfigValue & = default;

    [[nodiscard]] auto IsNull() const noexcept -> bool;
    [[nodiscard]] auto IsBool() const noexcept -> bool;
    [[nodiscard]] auto IsInt() const noexcept -> bool;
    [[nodiscard]] auto IsDouble() const noexcept -> bool;
    [[nodiscard]] auto IsString() const noexcept -> bool;

    [[nodiscard]] auto AsBool() const -> bool;
    [[nodiscard]] auto AsInt() const -> int64_t;
    [[nodiscard]] auto AsDouble() const -> double;
    [[nodiscard]] auto AsString() const -> const std::string &;
    [[nodiscard]] auto AsStringOpt() const noexcept -> const std::string *;
};

} // namespace silicon::config

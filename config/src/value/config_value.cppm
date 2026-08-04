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
  public:
    ConfigValue() noexcept = default;
    ~ConfigValue() = default;

    ConfigValue(std::nullptr_t) noexcept;
    ConfigValue(bool v) noexcept;
    ConfigValue(int64_t v) noexcept;
    ConfigValue(double v) noexcept;
    ConfigValue(std::string v) noexcept;

    ConfigValue(const ConfigValue &) = default;
    ConfigValue(ConfigValue &&) noexcept = default;
    auto operator=(const ConfigValue &) -> ConfigValue & = default;
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

  private:
    ConfigValueData data_{nullptr};
};

} // namespace silicon::config

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

    [[nodiscard]] auto is_null() const noexcept -> bool;
    [[nodiscard]] auto is_bool() const noexcept -> bool;
    [[nodiscard]] auto is_int() const noexcept -> bool;
    [[nodiscard]] auto is_double() const noexcept -> bool;
    [[nodiscard]] auto is_string() const noexcept -> bool;

    [[nodiscard]] auto as_bool() const -> bool;
    [[nodiscard]] auto as_int() const -> int64_t;
    [[nodiscard]] auto as_double() const -> double;
    [[nodiscard]] auto as_string() const -> const std::string &;
    [[nodiscard]] auto as_string_opt() const noexcept -> const std::string *;

  private:
    ConfigValueData m_data{nullptr};
};

} // namespace silicon::config

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
    struct P {
      public:
        ConfigValueData m_data{nullptr};
    };
    std::shared_ptr<P> m_p{std::make_shared<P>()};

  public:
    ConfigValue() = default;
    ~ConfigValue() = default;

    ConfigValue(std::nullptr_t);
    ConfigValue(bool v);
    ConfigValue(int64_t v);
    ConfigValue(double v);
    ConfigValue(std::string v);

    // 值类型语义：拷贝做深拷贝，不与源对象共享实现
    ConfigValue(const ConfigValue &o): m_p(std::make_shared<P>(*o.m_p)) {}
    auto operator=(const ConfigValue &o) -> ConfigValue & {
        if(this != &o) { m_p = std::make_shared<P>(*o.m_p); }
        return *this;
    }
    ConfigValue(ConfigValue &&) noexcept = default;
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
};

} // namespace silicon::config

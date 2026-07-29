module silicon.config.config_value;

#include <stdexcept>

namespace silicon::config {

ConfigValue::ConfigValue(std::nullptr_t) noexcept: m_data(nullptr) {}
ConfigValue::ConfigValue(bool v) noexcept: m_data(v) {}
ConfigValue::ConfigValue(int64_t v) noexcept: m_data(v) {}
ConfigValue::ConfigValue(double v) noexcept: m_data(v) {}
ConfigValue::ConfigValue(std::string v) noexcept: m_data(std::move(v)) {}

auto ConfigValue::is_null() const noexcept -> bool { return std::holds_alternative<std::nullptr_t>(m_data); }
auto ConfigValue::is_bool() const noexcept -> bool { return std::holds_alternative<bool>(m_data); }
auto ConfigValue::is_int() const noexcept -> bool { return std::holds_alternative<int64_t>(m_data); }
auto ConfigValue::is_double() const noexcept -> bool { return std::holds_alternative<double>(m_data); }
auto ConfigValue::is_string() const noexcept -> bool { return std::holds_alternative<std::string>(m_data); }

auto ConfigValue::as_bool() const -> bool { return std::get<bool>(m_data); }
auto ConfigValue::as_int() const -> int64_t { return std::get<int64_t>(m_data); }
auto ConfigValue::as_double() const -> double { return std::get<double>(m_data); }
auto ConfigValue::as_string() const -> const std::string & { return std::get<std::string>(m_data); }
auto ConfigValue::as_string_opt() const noexcept -> const std::string * {
    if(auto *p = std::get_if<std::string>(&m_data)) return p;
    return nullptr;
}

} // namespace silicon::config

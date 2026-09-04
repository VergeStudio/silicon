module;


#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

module silicon.config.config_value;

namespace silicon::config {

config_value::config_value(std::nullptr_t) { impl_->data_ = nullptr; }
config_value::config_value(bool v) { impl_->data_ = v; }
config_value::config_value(int64_t v) { impl_->data_ = v; }
config_value::config_value(double v) { impl_->data_ = v; }
config_value::config_value(std::string v) { impl_->data_ = std::move(v); }

bool config_value::is_null() const noexcept { return std::holds_alternative<std::nullptr_t>(impl_->data_); }
bool config_value::is_bool() const noexcept { return std::holds_alternative<bool>(impl_->data_); }
bool config_value::is_int() const noexcept { return std::holds_alternative<int64_t>(impl_->data_); }
bool config_value::is_double() const noexcept { return std::holds_alternative<double>(impl_->data_); }
bool config_value::is_string() const noexcept { return std::holds_alternative<std::string>(impl_->data_); }

bool config_value::as_bool() const { return std::get<bool>(impl_->data_); }
auto config_value::as_int() const -> int64_t { return std::get<int64_t>(impl_->data_); }
double config_value::as_double() const { return std::get<double>(impl_->data_); }
auto config_value::as_string() const -> const std::string & { return std::get<std::string>(impl_->data_); }
auto config_value::as_string_opt() const noexcept -> const std::string * {
    if(auto *p = std::get_if<std::string>(&impl_->data_)) return p;
    return nullptr;
}

}

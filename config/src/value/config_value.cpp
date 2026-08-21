module;

// 标准库头必须置于全局模块片段（module 声明之前）
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

bool config_value::IsNull() const noexcept { return std::holds_alternative<std::nullptr_t>(impl_->data_); }
bool config_value::IsBool() const noexcept { return std::holds_alternative<bool>(impl_->data_); }
bool config_value::IsInt() const noexcept { return std::holds_alternative<int64_t>(impl_->data_); }
bool config_value::IsDouble() const noexcept { return std::holds_alternative<double>(impl_->data_); }
bool config_value::IsString() const noexcept { return std::holds_alternative<std::string>(impl_->data_); }

bool config_value::AsBool() const { return std::get<bool>(impl_->data_); }
auto config_value::AsInt() const -> int64_t { return std::get<int64_t>(impl_->data_); }
double config_value::AsDouble() const { return std::get<double>(impl_->data_); }
auto config_value::AsString() const -> const std::string & { return std::get<std::string>(impl_->data_); }
auto config_value::AsStringOpt() const noexcept -> const std::string * {
    if(auto *p = std::get_if<std::string>(&impl_->data_)) return p;
    return nullptr;
}

} // namespace silicon::config

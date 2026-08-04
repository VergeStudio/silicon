module;

// 标准库头必须置于全局模块片段（module 声明之前）
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

module silicon.config.config_value;

namespace silicon::config {

ConfigValue::ConfigValue(std::nullptr_t) { impl_->data_ = nullptr; }
ConfigValue::ConfigValue(bool v) { impl_->data_ = v; }
ConfigValue::ConfigValue(int64_t v) { impl_->data_ = v; }
ConfigValue::ConfigValue(double v) { impl_->data_ = v; }
ConfigValue::ConfigValue(std::string v) { impl_->data_ = std::move(v); }

auto ConfigValue::IsNull() const noexcept -> bool { return std::holds_alternative<std::nullptr_t>(impl_->data_); }
auto ConfigValue::IsBool() const noexcept -> bool { return std::holds_alternative<bool>(impl_->data_); }
auto ConfigValue::IsInt() const noexcept -> bool { return std::holds_alternative<int64_t>(impl_->data_); }
auto ConfigValue::IsDouble() const noexcept -> bool { return std::holds_alternative<double>(impl_->data_); }
auto ConfigValue::IsString() const noexcept -> bool { return std::holds_alternative<std::string>(impl_->data_); }

auto ConfigValue::AsBool() const -> bool { return std::get<bool>(impl_->data_); }
auto ConfigValue::AsInt() const -> int64_t { return std::get<int64_t>(impl_->data_); }
auto ConfigValue::AsDouble() const -> double { return std::get<double>(impl_->data_); }
auto ConfigValue::AsString() const -> const std::string & { return std::get<std::string>(impl_->data_); }
auto ConfigValue::AsStringOpt() const noexcept -> const std::string * {
    if(auto *p = std::get_if<std::string>(&impl_->data_)) return p;
    return nullptr;
}

} // namespace silicon::config

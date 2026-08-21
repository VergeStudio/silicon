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
class config_value;

using config_value_data = std::variant<
    std::nullptr_t,
    bool,
    int64_t,
    double,
    std::string,
    std::shared_ptr<std::vector<config_value>>,
    std::shared_ptr<std::map<std::string, config_value>>>;

class CONFIG_API config_value {
    struct impl {
        config_value_data data_{nullptr};
    };
    std::shared_ptr<impl> impl_{std::make_shared<impl>()};

  public:
    config_value() = default;
    ~config_value() = default;

    config_value(std::nullptr_t);
    config_value(bool);
    config_value(int64_t);
    config_value(double);
    config_value(std::string);

    // 值类型语义：拷贝做深拷贝，不与源对象共享实现
    config_value(const config_value &o): impl_(std::make_shared<impl>(*o.impl_)) {}
    config_value & operator=(const config_value &o) {
        if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
        return *this;
    }
    config_value(config_value &&) noexcept = default;
    config_value & operator=(config_value &&) noexcept = default;

    [[nodiscard]] bool IsNull() const noexcept ;
    [[nodiscard]] bool IsBool() const noexcept ;
    [[nodiscard]] bool IsInt() const noexcept ;
    [[nodiscard]] bool IsDouble() const noexcept ;
    [[nodiscard]] bool IsString() const noexcept ;

    [[nodiscard]] bool AsBool() const ;
    [[nodiscard]] int64_t AsInt() const ;
    [[nodiscard]] double AsDouble() const ;
    [[nodiscard]] auto AsString() const -> const std::string &;
    [[nodiscard]] auto AsStringOpt() const noexcept -> const std::string *;
};

} // namespace silicon::config

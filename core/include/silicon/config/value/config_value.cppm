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

#include "silicon/common.h"

export module silicon.config.config_value;

export namespace silicon::config {

class config_value;

using config_value_data = std::variant<
    std::nullptr_t,
    bool,
    int64_t,
    double,
    std::string,
    std::shared_ptr<std::vector<config_value>>,
    std::shared_ptr<std::map<std::string, config_value>>>;

class SILICON_CORE_API config_value {
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

    config_value(const config_value &o): impl_(std::make_shared<impl>(*o.impl_)) {}
    config_value & operator=(const config_value &o) {
        if(this != &o) { impl_ = std::make_shared<impl>(*o.impl_); }
        return *this;
    }
    config_value(config_value &&) noexcept = default;
    config_value & operator=(config_value &&) noexcept = default;

    [[nodiscard]] bool is_null() const noexcept ;
    [[nodiscard]] bool is_bool() const noexcept ;
    [[nodiscard]] bool is_int() const noexcept ;
    [[nodiscard]] bool is_double() const noexcept ;
    [[nodiscard]] bool is_string() const noexcept ;

    [[nodiscard]] bool as_bool() const ;
    [[nodiscard]] int64_t as_int() const ;
    [[nodiscard]] double as_double() const ;
    [[nodiscard]] auto as_string() const -> const std::string &;
    [[nodiscard]] auto as_string_opt() const noexcept -> const std::string *;
};

}

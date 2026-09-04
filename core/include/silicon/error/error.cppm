module;

#include <expected>
#include <system_error>
#include <silicon/common.h>

export module silicon.error;

export namespace silicon::error {

template<typename T>
using result = std::expected<T, std::error_code>;

struct CORE_API category_deleter {
    void operator()(const std::error_category*) const noexcept {}
};

}

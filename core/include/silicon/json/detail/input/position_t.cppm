












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <cstddef>

export module silicon.json:detail.input.position_t;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {


export struct position_t {

    std::size_t chars_read_total = 0;

    std::size_t chars_read_current_line = 0;

    std::size_t lines_read = 0;


    constexpr operator size_t() const {
        return chars_read_total;
    }
};

}
SILICON_JSON_NAMESPACE_END

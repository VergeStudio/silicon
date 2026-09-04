module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#if JSON_HAS_THREE_WAY_COMPARISON
#    include <compare>
#endif

export module silicon.json:detail.value_t;

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export enum class value_t : std::uint8_t {
    null,
    object,
    array,
    string,
    boolean,
    number_integer,
    number_unsigned,
    number_float,
    binary,
    discarded
};

#if JSON_HAS_THREE_WAY_COMPARISON
export inline std::partial_ordering operator<=>(const value_t lhs, const value_t rhs) noexcept
#else
inline bool operator<(const value_t lhs, const value_t rhs) noexcept
#endif
{
    static constexpr std::array<std::uint8_t, 9> order = {{
            0 , 3 , 4 , 5 ,
            1 , 2 , 2 , 2 ,
            6 
    }};

    const auto l_index = static_cast<std::size_t>(lhs);
    const auto r_index = static_cast<std::size_t>(rhs);
#if JSON_HAS_THREE_WAY_COMPARISON
    if(l_index < order.size() && r_index < order.size()) {
        return order[l_index] <=> order[r_index];
    }
    return std::partial_ordering::unordered;
#else
    return l_index < order.size() && r_index < order.size() && order[l_index] < order[r_index];
#endif
}

#if JSON_HAS_THREE_WAY_COMPARISON && defined(__GNUC__)
export inline bool operator<(const value_t lhs, const value_t rhs) noexcept {
    return std::is_lt(lhs <=> rhs);
}
#endif

}
SILICON_JSON_NAMESPACE_END

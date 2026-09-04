module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>

export module silicon.json:detail.meta.void_t;

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export template<typename... Ts>
struct make_void {
    using type = void;
};
export template<typename... Ts>
using void_t = typename make_void<Ts...>::type;

}
SILICON_JSON_NAMESPACE_END

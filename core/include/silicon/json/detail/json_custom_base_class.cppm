module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <type_traits>

export module silicon.json:detail.json_custom_base_class;

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export struct json_default_base {};

export template<class T>
using json_base_class = typename std::conditional<
        std::is_same<T, void>::value,
        json_default_base,
        T>::type;

}
SILICON_JSON_NAMESPACE_END

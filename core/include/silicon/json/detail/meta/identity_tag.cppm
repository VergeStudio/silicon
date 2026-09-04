module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>

export module silicon.json:detail.meta.identity_tag;

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export template<class T>
struct identity_tag {};

}
SILICON_JSON_NAMESPACE_END

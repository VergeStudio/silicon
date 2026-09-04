












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>

export module silicon.json:detail.iterators.internal_iterator;

import :detail.iterators.primitive_iterator;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {


export template<typename BasicJsonType>
struct internal_iterator {

    typename BasicJsonType::object_t::iterator object_iterator{};

    typename BasicJsonType::array_t::iterator array_iterator{};

    primitive_iterator_t primitive_iterator{};
};

}
SILICON_JSON_NAMESPACE_END

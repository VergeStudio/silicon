












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <iterator>

export module silicon.json:detail.iterators.iterator_traits;

import :detail.meta.cpp_future;
import :detail.meta.void_t;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export template<typename It, typename = void>
struct iterator_types {};

export template<typename It>
struct iterator_types<
        It,
        void_t<typename It::difference_type, typename It::value_type, typename It::pointer, typename It::reference, typename It::iterator_category>> {
    using difference_type = typename It::difference_type;
    using value_type = typename It::value_type;
    using pointer = typename It::pointer;
    using reference = typename It::reference;
    using iterator_category = typename It::iterator_category;
};



export template<typename T, typename = void>
struct iterator_traits {
};

export template<typename T>
struct iterator_traits<T, enable_if_t<!std::is_pointer<T>::value>>
    : iterator_types<T> {
};

export template<typename T>
struct iterator_traits<T *, enable_if_t<std::is_object<T>::value>> {
    using iterator_category = std::random_access_iterator_tag;
    using value_type = T;
    using difference_type = ptrdiff_t;
    using pointer = T *;
    using reference = T &;
};

}
SILICON_JSON_NAMESPACE_END

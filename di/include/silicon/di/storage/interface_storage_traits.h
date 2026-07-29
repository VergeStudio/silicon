#pragma once

#include "silicon/di/core/config.h"

#include "silicon/di/registration/annotated.h"
#include "silicon/di/type/complete_type.h"
#include "silicon/di/type/rebind_type.h"
#include "silicon/di/type/type_list.h"
#include "silicon/di/type/type_traits.h"

#include <type_traits>

namespace silicon::di {
namespace detail {
template <typename Source, typename Target>
inline constexpr bool is_handle_rebindable_v =
    type_traits<Source>::enabled && type_traits<Source>::is_pointer_like &&
    type_traits<Source>::template is_handle_rebindable<Target>;
} // namespace detail

template <typename Storage, typename Interface>
inline constexpr bool is_interface_storage_rebindable_v =
    detail::is_handle_rebindable_v<
        Storage, rebind_leaf_t<Storage,
                               typename annotated_traits<Interface>::type>>;

namespace detail {
template <typename Storage, typename InterfaceList>
struct use_interface_as_stored_leaf;

template <typename Storage, typename... Interfaces>
struct use_interface_as_stored_leaf<Storage, type_list<Interfaces...>> : std::bool_constant<false> {};

template <typename Storage, typename Interface>
inline constexpr bool can_store_interface_as_leaf_v = [] {
    using interface_type = typename annotated_traits<Interface>::type;
    if constexpr (!requires_complete_type_v<interface_type>) {
        return false;
    } else if constexpr (!is_complete_v<interface_type>) {
        return false;
    } else if constexpr (!std::is_class_v<interface_type>) {
        return false;
    } else {
        return std::has_virtual_destructor_v<interface_type> &&
               is_interface_storage_rebindable_v<Storage, Interface>;
    }
}();

template <typename Storage, typename Interface>
struct use_interface_as_stored_leaf<Storage, type_list<Interface>>
    : std::bool_constant<can_store_interface_as_leaf_v<Storage, Interface>> {};

template <typename Storage, typename InterfaceList>
inline constexpr bool use_interface_as_stored_leaf_v =
    use_interface_as_stored_leaf<Storage, InterfaceList>::value;
} // namespace detail
} // namespace silicon::di

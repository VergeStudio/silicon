#pragma once

#include "silicon/di/core/none.h"
#include "silicon/di/resolution/type_cache.h"
#include "silicon/di/rtti/typeid_provider.h"
#include "silicon/di/type/type_list.h"
#include "silicon/di/type/type_map.h"

#include <memory>
#include <tuple>
#include <type_traits>

namespace silicon::di {

struct dynamic_container_traits {
    template <typename> using rebind_t = dynamic_container_traits;

    using tag_type = none_t;
    using rtti_type = rtti<typeid_provider>;
    template <typename Value, typename Allocator>
    using type_map_type = dynamic_type_map<Value, rtti_type, Allocator>;
    template <typename Value, typename Allocator>
    using type_cache_type = dynamic_type_cache<Value, rtti_type, Allocator>;
    using allocator_type = std::allocator<char>;
    using index_definition_type = std::tuple<>;
    static constexpr bool cache_enabled = true;
};

namespace detail {

template <typename T, typename = void>
struct is_runtime_container_traits : std::false_type {};

template <typename T>
struct is_runtime_container_traits<
    T, std::void_t<typename T::tag_type, typename T::rtti_type,
                   typename T::allocator_type,
                   typename T::index_definition_type,
                   typename T::template rebind_t<void>>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_runtime_container_traits_v =
    is_runtime_container_traits<T>::value;

template <typename Traits>
static constexpr bool is_tagged_container_v =
    !std::is_same_v<typename Traits::template rebind_t<
                        type_list<typename Traits::tag_type, void>>,
                    Traits>;

} // namespace detail
} // namespace silicon::di

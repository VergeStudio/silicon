#pragma once

#include "silicon/di/type/complete_type.h"

#include <type_traits>

namespace silicon::di {
namespace detail {

template <typename T, bool = is_complete<T>::value>
struct default_auto_constructible : std::false_type {};

template <typename T>
struct default_auto_constructible<T, true>
    : std::bool_constant<std::is_aggregate_v<T>> {};

} // namespace detail

template <typename T>
struct is_auto_constructible : detail::default_auto_constructible<T> {};

} // namespace silicon::di

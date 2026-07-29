#pragma once

#include "silicon/di/core/config.h"
#include "silicon/di/type/normalized_type.h"
#include "silicon/di/factory/constructor.h"

namespace silicon::di {
namespace detail {
template <typename StorageTag, typename Type, typename U> struct conversions;
template <typename StorageTag, typename Type, typename StoredType,
          typename Factory, typename Conversions>
class storage;

template <typename StorageTag, typename Type, typename StoredType,
          typename Factory>
class storage_instance;

template <typename StorageTag, typename Type, typename TypeInterface>
struct storage_interface_requirements : std::bool_constant<true> {};

template <typename Storage, typename Type, typename TypeInterface>
static constexpr bool storage_interface_requirements_v =
    storage_interface_requirements<Storage, Type, TypeInterface>::value;
} // namespace detail
} // namespace silicon::di

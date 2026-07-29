#pragma once

#include "silicon/di/core/config.h"

#include <cstddef>
#include <type_traits>

namespace silicon::di {
namespace detail {
template <typename Type> void destroy_object_value(Type& value) {
    if constexpr (std::is_array_v<Type>) {
        for (std::size_t i = std::extent_v<Type>; i > 0; --i) {
            destroy_object_value(value[i - 1]);
        }
    } else if constexpr (!std::is_trivially_destructible_v<Type>) {
        value.~Type();
    }
}
} // namespace detail
} // namespace silicon::di

#pragma once

#include "silicon/di/core/config.h"

#include "silicon/di/factory/constructor_traits.h"
#include "silicon/di/type/normalized_type.h"
#include "silicon/di/factory/constructor_detection.h"
#include "silicon/di/type/type_list.h"

namespace silicon::di {

template <typename...> struct constructor;

template <typename T> struct constructor<T> : constructor_detection<T> {};

template <typename T, typename... Args> struct constructor<T(Args...)> {
    using arguments = type_list<Args...>;
    static constexpr size_t arity = sizeof...(Args);
    static constexpr bool valid =
        detail::is_list_initializable_v<T, Args...> ||
        detail::is_direct_initializable_v<T, Args...>;

    template <typename Type, typename Context, typename Container>
    static auto construct(Context& ctx, Container& container) {
        return detail::construction_dispatch<Type, T>::construct(
            ctx.template resolve<Args>(container)...);
    }

    template <typename Type, typename Context, typename Container>
    static void construct(void* ptr, Context& ctx, Container& container) {
        detail::construction_dispatch<Type, T>::construct(
            ptr, ctx.template resolve<Args>(container)...);
    }
};

} // namespace silicon::di

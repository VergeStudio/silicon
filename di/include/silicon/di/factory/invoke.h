#pragma once

#include "silicon/di/core/config.h"
#include "silicon/di/factory/callable.h"

#include <utility>

namespace silicon::di {

template <typename T> struct invoke {
    template <typename Context, typename Container, typename Callable>
    static decltype(auto) construct(Context& ctx, Container& container,
                                    Callable&& callable) {
        return detail::callable_invoke<detail::callable_signature_t<T>>::
            construct(std::forward<Callable>(callable), ctx, container);
    }
};

} // namespace silicon::di

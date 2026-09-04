












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <type_traits>

export module silicon.json:detail.meta.detected;

import :detail.meta.void_t;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {


export struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(nonesuch const &) = delete;
    nonesuch(nonesuch const &&) = delete;
    void operator=(nonesuch const &) = delete;
    void operator=(nonesuch &&) = delete;
};

export template<class Default, class AlwaysVoid, template<class...> class Op, class... Args>
struct detector {
    using value_t = std::false_type;
    using type = Default;
};

export template<class Default, template<class...> class Op, class... Args>
struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type;
    using type = Op<Args...>;
};

export template<template<class...> class Op, class... Args>
using is_detected = typename detector<nonesuch, void, Op, Args...>::value_t;

export template<template<class...> class Op, class... Args>
struct is_detected_lazy: is_detected<Op, Args...> {};

export template<template<class...> class Op, class... Args>
using detected_t = typename detector<nonesuch, void, Op, Args...>::type;

export template<class Default, template<class...> class Op, class... Args>
using detected_or = detector<Default, void, Op, Args...>;

export template<class Default, template<class...> class Op, class... Args>
using detected_or_t = typename detected_or<Default, Op, Args...>::type;

export template<class Expected, template<class...> class Op, class... Args>
using is_detected_exact = std::is_same<Expected, detected_t<Op, Args...>>;

export template<class To, template<class...> class Op, class... Args>
using is_detected_convertible =
        std::is_convertible<detected_t<Op, Args...>, To>;

}
SILICON_JSON_NAMESPACE_END

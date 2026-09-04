












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <utility>

export module silicon.json:adl_serializer;

import :detail.conversions.from_json;
import :detail.conversions.to_json;
import :detail.meta.identity_tag;


SILICON_JSON_NAMESPACE_BEGIN


export template<typename ValueType, typename>
struct adl_serializer {


    template<typename BasicJsonType, typename TargetType = ValueType>
    static auto from_json(BasicJsonType &&j, TargetType &val) noexcept(
            noexcept(::silicon::json::impl::from_json(std::forward<BasicJsonType>(j), val))
    )
            -> decltype(::silicon::json::impl::from_json(std::forward<BasicJsonType>(j), val), void()) {
        ::silicon::json::impl::from_json(std::forward<BasicJsonType>(j), val);
    }



    template<typename BasicJsonType, typename TargetType = ValueType>
    static auto from_json(BasicJsonType &&j) noexcept(
            noexcept(::silicon::json::impl::from_json(std::forward<BasicJsonType>(j), detail::identity_tag<TargetType>{}))
    )
            -> decltype(::silicon::json::impl::from_json(std::forward<BasicJsonType>(j), detail::identity_tag<TargetType>{})) {
        return ::silicon::json::impl::from_json(std::forward<BasicJsonType>(j), detail::identity_tag<TargetType>{});
    }



    template<typename BasicJsonType, typename TargetType = ValueType>
    static auto to_json(BasicJsonType &j, TargetType &&val) noexcept(
            noexcept(::silicon::json::impl::to_json(j, std::forward<TargetType>(val)))
    )
            -> decltype(::silicon::json::impl::to_json(j, std::forward<TargetType>(val)), void()) {
        ::silicon::json::impl::to_json(j, std::forward<TargetType>(val));
    }
};

SILICON_JSON_NAMESPACE_END

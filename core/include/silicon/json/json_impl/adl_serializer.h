//     __ _____ _____ _____
//  __|  |   __|     |   | |  silicon JSON
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon
//
// SPDX-FileCopyrightText: silicon contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <silicon/json/json_impl/detail/abi_macros.h>
#include <silicon/json/json_impl/detail/conversions/from_json.h>
#include <silicon/json/json_impl/detail/conversions/to_json.h>
#include <silicon/json/json_impl/detail/meta/identity_tag.h>
#include <utility>

SILICON_JSON_NAMESPACE_BEGIN

/// @sa https://json.silicon.me/api/adl_serializer/
template<typename ValueType, typename>
struct adl_serializer {
    /// @brief convert a JSON value to any value type
    /// @sa https://json.silicon.me/api/adl_serializer/from_json/
    template<typename BasicJsonType, typename TargetType = ValueType>
    static auto from_json(BasicJsonType &&j, TargetType &val) noexcept(
            noexcept(::silicon::json_impl::from_json(std::forward<BasicJsonType>(j), val))
    )
            -> decltype(::silicon::json_impl::from_json(std::forward<BasicJsonType>(j), val), void()) {
        ::silicon::json_impl::from_json(std::forward<BasicJsonType>(j), val);
    }

    /// @brief convert a JSON value to any value type
    /// @sa https://json.silicon.me/api/adl_serializer/from_json/
    template<typename BasicJsonType, typename TargetType = ValueType>
    static auto from_json(BasicJsonType &&j) noexcept(
            noexcept(::silicon::json_impl::from_json(std::forward<BasicJsonType>(j), detail::identity_tag<TargetType>{}))
    )
            -> decltype(::silicon::json_impl::from_json(std::forward<BasicJsonType>(j), detail::identity_tag<TargetType>{})) {
        return ::silicon::json_impl::from_json(std::forward<BasicJsonType>(j), detail::identity_tag<TargetType>{});
    }

    /// @brief convert any value type to a JSON value
    /// @sa https://json.silicon.me/api/adl_serializer/to_json/
    template<typename BasicJsonType, typename TargetType = ValueType>
    static auto to_json(BasicJsonType &j, TargetType &&val) noexcept(
            noexcept(::silicon::json_impl::to_json(j, std::forward<TargetType>(val)))
    )
            -> decltype(::silicon::json_impl::to_json(j, std::forward<TargetType>(val)), void()) {
        ::silicon::json_impl::to_json(j, std::forward<TargetType>(val));
    }
};

SILICON_JSON_NAMESPACE_END

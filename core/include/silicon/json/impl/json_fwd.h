//     __ _____ _____ _____
//  __|  |   __|     |   | |  silicon JSON
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon
//
// SPDX-FileCopyrightText: silicon contributors
// SPDX-License-Identifier: MIT

#ifndef INCLUDE_SILICON_JSON_FWD_HPP_
#define INCLUDE_SILICON_JSON_FWD_HPP_

#include <cstdint> // int64_t, uint64_t
#include <map>     // map
#include <memory>  // allocator
#include <silicon/json/impl/detail/abi_macros.h>
#include <string> // string
#include <vector> // vector

/*!
@brief namespace for silicon JSON
@see https://github.com/silicon
@since version 1.0.0
*/
SILICON_JSON_NAMESPACE_BEGIN

/*!
@brief default JSONSerializer template argument

This serializer ignores the template arguments and uses ADL
([argument-dependent lookup](https://en.cppreference.com/w/cpp/language/adl))
for serialization.
*/
template<typename T = void, typename SFINAE = void>
struct adl_serializer;

/// a class to store JSON values
/// @sa https://json.silicon.me/api/basic_json/
template<template<typename U, typename V, typename... Args> class ObjectType = std::map, template<typename U, typename... Args> class ArrayType = std::vector, class StringType = std::string, class BooleanType = bool, class NumberIntegerType = std::int64_t, class NumberUnsignedType = std::uint64_t, class NumberFloatType = double, template<typename U> class AllocatorType = std::allocator, template<typename T, typename SFINAE = void> class JSONSerializer = adl_serializer,
         class BinaryType = std::vector<std::uint8_t>, // cppcheck-suppress syntaxError
         class CustomBaseClass = void>
class basic_json;

/// @brief JSON Pointer defines a string syntax for identifying a specific value within a JSON document
/// @sa https://json.silicon.me/api/json_pointer/
template<typename RefStringType>
class json_pointer;

/*!
@brief default specialization
@sa https://json.silicon.me/api/json/
*/
using json = basic_json<>;

/// @brief a minimal map-like container that preserves insertion order
/// @sa https://json.silicon.me/api/ordered_map/
template<class Key, class T, class IgnoredLess, class Allocator>
struct ordered_map;

/// @brief specialization that maintains the insertion order of object keys
/// @sa https://json.silicon.me/api/ordered_json/
using ordered_json = basic_json<silicon::json::impl::ordered_map>;

SILICON_JSON_NAMESPACE_END

#endif // INCLUDE_SILICON_JSON_FWD_HPP_

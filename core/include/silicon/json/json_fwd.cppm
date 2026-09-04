module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

export module silicon.json:json_fwd;

export {

SILICON_JSON_NAMESPACE_BEGIN

template<typename T = void, typename SFINAE = void>
struct adl_serializer;

template<template<typename U, typename V, typename... Args> class ObjectType = std::map, template<typename U, typename... Args> class ArrayType = std::vector, class StringType = std::string, class BooleanType = bool, class NumberIntegerType = std::int64_t, class NumberUnsignedType = std::uint64_t, class NumberFloatType = double, template<typename U> class AllocatorType = std::allocator, template<typename T, typename SFINAE = void> class JSONSerializer = adl_serializer,
         class BinaryType = std::vector<std::uint8_t>,
         class CustomBaseClass = void>
class basic_json;

template<typename RefStringType>
class json_pointer;

using json = basic_json<>;

template<class Key, class T, class IgnoredLess, class Allocator>
struct ordered_map;

using ordered_json = basic_json<silicon::json::impl::ordered_map>;

SILICON_JSON_NAMESPACE_END
}

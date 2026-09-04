












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#if JSON_HAS_EXPERIMENTAL_FILESYSTEM
#    include <experimental/filesystem>
#elif JSON_HAS_FILESYSTEM
#    include <filesystem>
#endif
#include <algorithm>
#include <array>
#include <forward_list>
#include <iterator>
#include <map>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <valarray>

export module silicon.json:detail.conversions.from_json;

import :detail.exceptions;
import :detail.meta.detected;
import :detail.meta.cpp_future;
import :detail.meta.identity_tag;
import :detail.meta.std_fs;
import :detail.meta.type_traits;
import :detail.string_concat;
import :detail.value_t;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export template<typename BasicJsonType>
inline void from_json(const BasicJsonType &j, typename std::nullptr_t &n) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_null())) {
        JSON_THROW(type_error::create(302, concat("type must be null, but is ", j.type_name()), &j));
    }
    n = nullptr;
}


export template<typename BasicJsonType, typename ArithmeticType, enable_if_t<std::is_arithmetic<ArithmeticType>::value && !std::is_same<ArithmeticType, typename BasicJsonType::boolean_t>::value, int> = 0>
void get_arithmetic_value(const BasicJsonType &j, ArithmeticType &val) {
    switch(static_cast<value_t>(j)) {
        case value_t::number_unsigned: {
            val = static_cast<ArithmeticType>(*j.template get_ptr<const typename BasicJsonType::number_unsigned_t *>());
            break;
        }
        case value_t::number_integer: {
            val = static_cast<ArithmeticType>(*j.template get_ptr<const typename BasicJsonType::number_integer_t *>());
            break;
        }
        case value_t::number_float: {
            val = static_cast<ArithmeticType>(*j.template get_ptr<const typename BasicJsonType::number_float_t *>());
            break;
        }

        case value_t::null:
        case value_t::object:
        case value_t::array:
        case value_t::string:
        case value_t::boolean:
        case value_t::binary:
        case value_t::discarded:
        default:
            JSON_THROW(type_error::create(302, concat("type must be number, but is ", j.type_name()), &j));
    }
}

export template<typename BasicJsonType>
inline void from_json(const BasicJsonType &j, typename BasicJsonType::boolean_t &b) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_boolean())) {
        JSON_THROW(type_error::create(302, concat("type must be boolean, but is ", j.type_name()), &j));
    }
    b = *j.template get_ptr<const typename BasicJsonType::boolean_t *>();
}

export template<typename BasicJsonType>
inline void from_json(const BasicJsonType &j, typename BasicJsonType::string_t &s) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_string())) {
        JSON_THROW(type_error::create(302, concat("type must be string, but is ", j.type_name()), &j));
    }
    s = *j.template get_ptr<const typename BasicJsonType::string_t *>();
}

export template<
        typename BasicJsonType,
        typename StringType,
        enable_if_t<
                std::is_assignable<StringType &, const typename BasicJsonType::string_t>::value && is_detected_exact<typename BasicJsonType::string_t::value_type, value_type_t, StringType>::value && !std::is_same<typename BasicJsonType::string_t, StringType>::value && !is_json_ref<StringType>::value,
                int> = 0>
inline void from_json(const BasicJsonType &j, StringType &s) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_string())) {
        JSON_THROW(type_error::create(302, concat("type must be string, but is ", j.type_name()), &j));
    }

    s = *j.template get_ptr<const typename BasicJsonType::string_t *>();
}

export template<typename BasicJsonType>
inline void from_json(const BasicJsonType &j, typename BasicJsonType::number_float_t &val) {
    get_arithmetic_value(j, val);
}

export template<typename BasicJsonType>
inline void from_json(const BasicJsonType &j, typename BasicJsonType::number_unsigned_t &val) {
    get_arithmetic_value(j, val);
}

export template<typename BasicJsonType>
inline void from_json(const BasicJsonType &j, typename BasicJsonType::number_integer_t &val) {
    get_arithmetic_value(j, val);
}

#if !JSON_DISABLE_ENUM_SERIALIZATION
export template<typename BasicJsonType, typename EnumType, enable_if_t<std::is_enum<EnumType>::value, int> = 0>
inline void from_json(const BasicJsonType &j, EnumType &e) {
    typename std::underlying_type<EnumType>::type val;
    get_arithmetic_value(j, val);
    e = static_cast<EnumType>(val);
}
#endif


export template<typename BasicJsonType, typename T, typename Allocator, enable_if_t<is_getable<BasicJsonType, T>::value, int> = 0>
inline void from_json(const BasicJsonType &j, std::forward_list<T, Allocator> &l) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_array())) {
        JSON_THROW(type_error::create(302, concat("type must be array, but is ", j.type_name()), &j));
    }
    l.clear();
    std::transform(j.rbegin(), j.rend(), std::front_inserter(l), [](const BasicJsonType &i) {
        return i.template get<T>();
    });
}


export template<typename BasicJsonType, typename T, enable_if_t<is_getable<BasicJsonType, T>::value, int> = 0>
inline void from_json(const BasicJsonType &j, std::valarray<T> &l) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_array())) {
        JSON_THROW(type_error::create(302, concat("type must be array, but is ", j.type_name()), &j));
    }
    l.resize(j.size());
    std::transform(j.begin(), j.end(), std::begin(l), [](const BasicJsonType &elem) {
        return elem.template get<T>();
    });
}

export template<typename BasicJsonType, typename T, std::size_t N>
auto from_json(const BasicJsonType &j, T (&arr)[N])
        -> decltype(j.template get<T>(), void()) {
    for(std::size_t i = 0; i < N; ++i) {
        arr[i] = j.at(i).template get<T>();
    }
}

export template<typename BasicJsonType>
inline void from_json_array_impl(const BasicJsonType &j, typename BasicJsonType::array_t &arr, priority_tag<3> ) {
    arr = *j.template get_ptr<const typename BasicJsonType::array_t *>();
}

export template<typename BasicJsonType, typename T, std::size_t N>
auto from_json_array_impl(const BasicJsonType &j, std::array<T, N> &arr, priority_tag<2> )
        -> decltype(j.template get<T>(), void()) {
    for(std::size_t i = 0; i < N; ++i) {
        arr[i] = j.at(i).template get<T>();
    }
}

export template<typename BasicJsonType, typename ConstructibleArrayType, enable_if_t<std::is_assignable<ConstructibleArrayType &, ConstructibleArrayType>::value, int> = 0>
auto from_json_array_impl(const BasicJsonType &j, ConstructibleArrayType &arr, priority_tag<1> )
        -> decltype(arr.reserve(std::declval<typename ConstructibleArrayType::size_type>()), j.template get<typename ConstructibleArrayType::value_type>(), void()) {
    using std::end;

    ConstructibleArrayType ret;
    ret.reserve(j.size());
    std::transform(j.begin(), j.end(), std::inserter(ret, end(ret)), [](const BasicJsonType &i) {


        return i.template get<typename ConstructibleArrayType::value_type>();
    });
    arr = std::move(ret);
}

export template<typename BasicJsonType, typename ConstructibleArrayType, enable_if_t<std::is_assignable<ConstructibleArrayType &, ConstructibleArrayType>::value, int> = 0>
inline void from_json_array_impl(const BasicJsonType &j, ConstructibleArrayType &arr, priority_tag<0> ) {
    using std::end;

    ConstructibleArrayType ret;
    std::transform(
            j.begin(), j.end(), std::inserter(ret, end(ret)),
            [](const BasicJsonType &i) {


                return i.template get<typename ConstructibleArrayType::value_type>();
            }
    );
    arr = std::move(ret);
}

export template<typename BasicJsonType, typename ConstructibleArrayType, enable_if_t<is_constructible_array_type<BasicJsonType, ConstructibleArrayType>::value && !is_constructible_object_type<BasicJsonType, ConstructibleArrayType>::value && !is_constructible_string_type<BasicJsonType, ConstructibleArrayType>::value && !std::is_same<ConstructibleArrayType, typename BasicJsonType::binary_t>::value && !is_basic_json<ConstructibleArrayType>::value, int> = 0>
auto from_json(const BasicJsonType &j, ConstructibleArrayType &arr)
        -> decltype(from_json_array_impl(j, arr, priority_tag<3>{}), j.template get<typename ConstructibleArrayType::value_type>(), void()) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_array())) {
        JSON_THROW(type_error::create(302, concat("type must be array, but is ", j.type_name()), &j));
    }

    from_json_array_impl(j, arr, priority_tag<3>{});
}

export template<typename BasicJsonType, typename T, std::size_t... Idx>
std::array<T, sizeof...(Idx)> from_json_inplace_array_impl(BasicJsonType &&j, identity_tag<std::array<T, sizeof...(Idx)>> , index_sequence<Idx...> ) {
    return {{std::forward<BasicJsonType>(j).at(Idx).template get<T>()...}};
}

export template<typename BasicJsonType, typename T, std::size_t N>
auto from_json(BasicJsonType &&j, identity_tag<std::array<T, N>> tag)
        -> decltype(from_json_inplace_array_impl(std::forward<BasicJsonType>(j), tag, make_index_sequence<N>{})) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_array())) {
        JSON_THROW(type_error::create(302, concat("type must be array, but is ", j.type_name()), &j));
    }

    return from_json_inplace_array_impl(std::forward<BasicJsonType>(j), tag, make_index_sequence<N>{});
}

export template<typename BasicJsonType>
inline void from_json(const BasicJsonType &j, typename BasicJsonType::binary_t &bin) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_binary())) {
        JSON_THROW(type_error::create(302, concat("type must be binary, but is ", j.type_name()), &j));
    }

    bin = *j.template get_ptr<const typename BasicJsonType::binary_t *>();
}

export template<typename BasicJsonType, typename ConstructibleObjectType, enable_if_t<is_constructible_object_type<BasicJsonType, ConstructibleObjectType>::value, int> = 0>
inline void from_json(const BasicJsonType &j, ConstructibleObjectType &obj) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_object())) {
        JSON_THROW(type_error::create(302, concat("type must be object, but is ", j.type_name()), &j));
    }

    ConstructibleObjectType ret;
    const auto *inner_object = j.template get_ptr<const typename BasicJsonType::object_t *>();
    using value_type = typename ConstructibleObjectType::value_type;
    std::transform(
            inner_object->begin(), inner_object->end(),
            std::inserter(ret, ret.begin()),
            [](typename BasicJsonType::object_t::value_type const &p) {
                return value_type(p.first, p.second.template get<typename ConstructibleObjectType::mapped_type>());
            }
    );
    obj = std::move(ret);
}





export template<typename BasicJsonType, typename ArithmeticType, enable_if_t<std::is_arithmetic<ArithmeticType>::value && !std::is_same<ArithmeticType, typename BasicJsonType::number_unsigned_t>::value && !std::is_same<ArithmeticType, typename BasicJsonType::number_integer_t>::value && !std::is_same<ArithmeticType, typename BasicJsonType::number_float_t>::value && !std::is_same<ArithmeticType, typename BasicJsonType::boolean_t>::value, int> = 0>
inline void from_json(const BasicJsonType &j, ArithmeticType &val) {
    switch(static_cast<value_t>(j)) {
        case value_t::number_unsigned: {
            val = static_cast<ArithmeticType>(*j.template get_ptr<const typename BasicJsonType::number_unsigned_t *>());
            break;
        }
        case value_t::number_integer: {
            val = static_cast<ArithmeticType>(*j.template get_ptr<const typename BasicJsonType::number_integer_t *>());
            break;
        }
        case value_t::number_float: {
            val = static_cast<ArithmeticType>(*j.template get_ptr<const typename BasicJsonType::number_float_t *>());
            break;
        }
        case value_t::boolean: {
            val = static_cast<ArithmeticType>(*j.template get_ptr<const typename BasicJsonType::boolean_t *>());
            break;
        }

        case value_t::null:
        case value_t::object:
        case value_t::array:
        case value_t::string:
        case value_t::binary:
        case value_t::discarded:
        default:
            JSON_THROW(type_error::create(302, concat("type must be number, but is ", j.type_name()), &j));
    }
}

export template<typename BasicJsonType, typename... Args, std::size_t... Idx>
std::tuple<Args...> from_json_tuple_impl_base(BasicJsonType &&j, index_sequence<Idx...> ) {
    return std::make_tuple(std::forward<BasicJsonType>(j).at(Idx).template get<Args>()...);
}

export template<typename BasicJsonType, class A1, class A2>
std::pair<A1, A2> from_json_tuple_impl(BasicJsonType &&j, identity_tag<std::pair<A1, A2>> , priority_tag<0> ) {
    return {std::forward<BasicJsonType>(j).at(0).template get<A1>(),
            std::forward<BasicJsonType>(j).at(1).template get<A2>()};
}

export template<typename BasicJsonType, typename A1, typename A2>
inline void from_json_tuple_impl(BasicJsonType &&j, std::pair<A1, A2> &p, priority_tag<1> ) {
    p = from_json_tuple_impl(std::forward<BasicJsonType>(j), identity_tag<std::pair<A1, A2>>{}, priority_tag<0>{});
}

export template<typename BasicJsonType, typename... Args>
std::tuple<Args...> from_json_tuple_impl(BasicJsonType &&j, identity_tag<std::tuple<Args...>> , priority_tag<2> ) {
    return from_json_tuple_impl_base<BasicJsonType, Args...>(std::forward<BasicJsonType>(j), index_sequence_for<Args...>{});
}

export template<typename BasicJsonType, typename... Args>
inline void from_json_tuple_impl(BasicJsonType &&j, std::tuple<Args...> &t, priority_tag<3> ) {
    t = from_json_tuple_impl_base<BasicJsonType, Args...>(std::forward<BasicJsonType>(j), index_sequence_for<Args...>{});
}

export template<typename BasicJsonType, typename TupleRelated>
auto from_json(BasicJsonType &&j, TupleRelated &&t)
        -> decltype(from_json_tuple_impl(std::forward<BasicJsonType>(j), std::forward<TupleRelated>(t), priority_tag<3>{})) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_array())) {
        JSON_THROW(type_error::create(302, concat("type must be array, but is ", j.type_name()), &j));
    }

    return from_json_tuple_impl(std::forward<BasicJsonType>(j), std::forward<TupleRelated>(t), priority_tag<3>{});
}

export template<typename BasicJsonType, typename Key, typename Value, typename Compare, typename Allocator, typename = enable_if_t<!std::is_constructible<typename BasicJsonType::string_t, Key>::value>>
inline void from_json(const BasicJsonType &j, std::map<Key, Value, Compare, Allocator> &m) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_array())) {
        JSON_THROW(type_error::create(302, concat("type must be array, but is ", j.type_name()), &j));
    }
    m.clear();
    for(const auto &p: j) {
        if(JSON_HEDLEY_UNLIKELY(!p.is_array())) {
            JSON_THROW(type_error::create(302, concat("type must be array, but is ", p.type_name()), &j));
        }
        m.emplace(p.at(0).template get<Key>(), p.at(1).template get<Value>());
    }
}

export template<typename BasicJsonType, typename Key, typename Value, typename Hash, typename KeyEqual, typename Allocator, typename = enable_if_t<!std::is_constructible<typename BasicJsonType::string_t, Key>::value>>
inline void from_json(const BasicJsonType &j, std::unordered_map<Key, Value, Hash, KeyEqual, Allocator> &m) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_array())) {
        JSON_THROW(type_error::create(302, concat("type must be array, but is ", j.type_name()), &j));
    }
    m.clear();
    for(const auto &p: j) {
        if(JSON_HEDLEY_UNLIKELY(!p.is_array())) {
            JSON_THROW(type_error::create(302, concat("type must be array, but is ", p.type_name()), &j));
        }
        m.emplace(p.at(0).template get<Key>(), p.at(1).template get<Value>());
    }
}

#if JSON_HAS_FILESYSTEM || JSON_HAS_EXPERIMENTAL_FILESYSTEM
export template<typename BasicJsonType>
inline void from_json(const BasicJsonType &j, std_fs::path &p) {
    if(JSON_HEDLEY_UNLIKELY(!j.is_string())) {
        JSON_THROW(type_error::create(302, concat("type must be string, but is ", j.type_name()), &j));
    }
    p = *j.template get_ptr<const typename BasicJsonType::string_t *>();
}
#endif

export struct from_json_fn {
    template<typename BasicJsonType, typename T>
    auto operator()(const BasicJsonType &j, T &&val) const noexcept(noexcept(from_json(j, std::forward<T>(val))))
            -> decltype(from_json(j, std::forward<T>(val))) {
        return from_json(j, std::forward<T>(val));
    }
};

}

#ifndef JSON_HAS_CPP_17



namespace
export {
#endif
JSON_INLINE_VARIABLE constexpr const auto &from_json =
        detail::static_const<detail::from_json_fn>::value;
#ifndef JSON_HAS_CPP_17
}
#endif

SILICON_JSON_NAMESPACE_END

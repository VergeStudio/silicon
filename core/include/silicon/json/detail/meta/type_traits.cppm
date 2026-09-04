module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <limits>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

export module silicon.json:detail.meta.type_traits;

import :detail.iterators.iterator_traits;
import :detail.meta.call_std.begin;
import :detail.meta.call_std.end;
import :detail.meta.cpp_future;
import :detail.meta.detected;
import :json_fwd;

SILICON_JSON_NAMESPACE_BEGIN

namespace detail {

export template<typename>
struct is_basic_json: std::false_type {};

export silicon_BASIC_JSON_TPL_DECLARATION struct is_basic_json<silicon_BASIC_JSON_TPL>: std::true_type {};

export template<typename BasicJsonContext>
struct is_basic_json_context: std::integral_constant<bool, is_basic_json<typename std::remove_cv<typename std::remove_pointer<BasicJsonContext>::type>::type>::value || std::is_same<BasicJsonContext, std::nullptr_t>::value> {};

export template<typename>
class json_ref;

export template<typename>
struct is_json_ref: std::false_type {};

export template<typename T>
struct is_json_ref<json_ref<T>>: std::true_type {};

export template<typename T>
using mapped_type_t = typename T::mapped_type;

export template<typename T>
using key_type_t = typename T::key_type;

export template<typename T>
using value_type_t = typename T::value_type;

export template<typename T>
using difference_type_t = typename T::difference_type;

export template<typename T>
using pointer_t = typename T::pointer;

export template<typename T>
using reference_t = typename T::reference;

export template<typename T>
using iterator_category_t = typename T::iterator_category;

export template<typename T, typename... Args>
using to_json_function = decltype(T::to_json(std::declval<Args>()...));

export template<typename T, typename... Args>
using from_json_function = decltype(T::from_json(std::declval<Args>()...));

export template<typename T, typename U>
using get_template_function = decltype(std::declval<T>().template get<U>());

export template<typename BasicJsonType, typename T, typename = void>
struct has_from_json: std::false_type {};

export template<typename BasicJsonType, typename T>
struct is_getable {
    static constexpr bool value = is_detected<get_template_function, const BasicJsonType &, T>::value;
};

export template<typename BasicJsonType, typename T>
struct has_from_json<BasicJsonType, T, enable_if_t<!is_basic_json<T>::value>> {
    using serializer = typename BasicJsonType::template json_serializer<T, void>;

    static constexpr bool value =
            is_detected_exact<void, from_json_function, serializer, const BasicJsonType &, T &>::value;
};

export template<typename BasicJsonType, typename T, typename = void>
struct has_non_default_from_json: std::false_type {};

export template<typename BasicJsonType, typename T>
struct has_non_default_from_json<BasicJsonType, T, enable_if_t<!is_basic_json<T>::value>> {
    using serializer = typename BasicJsonType::template json_serializer<T, void>;

    static constexpr bool value =
            is_detected_exact<T, from_json_function, serializer, const BasicJsonType &>::value;
};

export template<typename BasicJsonType, typename T, typename = void>
struct has_to_json: std::false_type {};

export template<typename BasicJsonType, typename T>
struct has_to_json<BasicJsonType, T, enable_if_t<!is_basic_json<T>::value>> {
    using serializer = typename BasicJsonType::template json_serializer<T, void>;

    static constexpr bool value =
            is_detected_exact<void, to_json_function, serializer, BasicJsonType &, T>::value;
};

export template<typename T>
using detect_key_compare = typename T::key_compare;

export template<typename T>
struct has_key_compare: std::integral_constant<bool, is_detected<detect_key_compare, T>::value> {};

export template<typename BasicJsonType>
struct actual_object_comparator {
    using object_t = typename BasicJsonType::object_t;
    using object_comparator_t = typename BasicJsonType::default_object_comparator_t;
    using type = typename std::conditional<has_key_compare<object_t>::value, typename object_t::key_compare, object_comparator_t>::type;
};

export template<typename BasicJsonType>
using actual_object_comparator_t = typename actual_object_comparator<BasicJsonType>::type;

export template<typename T>
struct char_traits: std::char_traits<T> {};

export template<>
struct char_traits<unsigned char>: std::char_traits<char> {
    using char_type = unsigned char;
    using int_type = uint64_t;

    static int_type to_int_type(char_type c) noexcept {
        return static_cast<int_type>(c);
    }

    static char_type to_char_type(int_type i) noexcept {
        return static_cast<char_type>(i);
    }

    static constexpr int_type eof() noexcept {
        return static_cast<int_type>(EOF);
    }
};

export template<>
struct char_traits<signed char>: std::char_traits<char> {
    using char_type = signed char;
    using int_type = uint64_t;

    static int_type to_int_type(char_type c) noexcept {
        return static_cast<int_type>(c);
    }

    static char_type to_char_type(int_type i) noexcept {
        return static_cast<char_type>(i);
    }

    static constexpr int_type eof() noexcept {
        return static_cast<int_type>(EOF);
    }
};

export template<class...>
struct conjunction: std::true_type {};
export template<class B>
struct conjunction<B>: B {};
export template<class B, class... Bn>
struct conjunction<B, Bn...>
    : std::conditional<static_cast<bool>(B::value), conjunction<Bn...>, B>::type {};

export template<class B>
struct negation: std::integral_constant<bool, !B::value> {};

export template<typename T>
struct is_default_constructible: std::is_default_constructible<T> {};

export template<typename T1, typename T2>
struct is_default_constructible<std::pair<T1, T2>>
    : conjunction<is_default_constructible<T1>, is_default_constructible<T2>> {};

export template<typename T1, typename T2>
struct is_default_constructible<const std::pair<T1, T2>>
    : conjunction<is_default_constructible<T1>, is_default_constructible<T2>> {};

export template<typename... Ts>
struct is_default_constructible<std::tuple<Ts...>>
    : conjunction<is_default_constructible<Ts>...> {};

export template<typename... Ts>
struct is_default_constructible<const std::tuple<Ts...>>
    : conjunction<is_default_constructible<Ts>...> {};

export template<typename T, typename... Args>
struct is_constructible: std::is_constructible<T, Args...> {};

export template<typename T1, typename T2>
struct is_constructible<std::pair<T1, T2>>: is_default_constructible<std::pair<T1, T2>> {};

export template<typename T1, typename T2>
struct is_constructible<const std::pair<T1, T2>>: is_default_constructible<const std::pair<T1, T2>> {};

export template<typename... Ts>
struct is_constructible<std::tuple<Ts...>>: is_default_constructible<std::tuple<Ts...>> {};

export template<typename... Ts>
struct is_constructible<const std::tuple<Ts...>>: is_default_constructible<const std::tuple<Ts...>> {};

export template<typename T, typename = void>
struct is_iterator_traits: std::false_type {};

export template<typename T>
struct is_iterator_traits<iterator_traits<T>> {
  private:
    using traits = iterator_traits<T>;

  public:
    static constexpr auto value =
            is_detected<value_type_t, traits>::value &&
            is_detected<difference_type_t, traits>::value &&
            is_detected<pointer_t, traits>::value &&
            is_detected<iterator_category_t, traits>::value &&
            is_detected<reference_t, traits>::value;
};

export template<typename T>
struct is_range {
  private:
    using t_ref = typename std::add_lvalue_reference<T>::type;

    using iterator = detected_t<result_of_begin, t_ref>;
    using sentinel = detected_t<result_of_end, t_ref>;

    static constexpr auto is_iterator_begin =
            is_iterator_traits<iterator_traits<iterator>>::value;

  public:
    static constexpr bool value = !std::is_same<iterator, nonesuch>::value && !std::is_same<sentinel, nonesuch>::value && is_iterator_begin;
};

export template<typename R>
using iterator_t = enable_if_t<is_range<R>::value, result_of_begin<decltype(std::declval<R &>())>>;

export template<typename T>
using range_value_t = value_type_t<iterator_traits<iterator_t<T>>>;

export template<typename T, typename = void>
struct is_complete_type: std::false_type {};

export template<typename T>
struct is_complete_type<T, decltype(void(sizeof(T)))>: std::true_type {};

export template<typename BasicJsonType, typename CompatibleObjectType, typename = void>
struct is_compatible_object_type_impl: std::false_type {};

export template<typename BasicJsonType, typename CompatibleObjectType>
struct is_compatible_object_type_impl<
        BasicJsonType,
        CompatibleObjectType,
        enable_if_t<is_detected<mapped_type_t, CompatibleObjectType>::value && is_detected<key_type_t, CompatibleObjectType>::value>> {
    using object_t = typename BasicJsonType::object_t;

    static constexpr bool value =
            is_constructible<typename object_t::key_type, typename CompatibleObjectType::key_type>::value &&
            is_constructible<typename object_t::mapped_type, typename CompatibleObjectType::mapped_type>::value;
};

export template<typename BasicJsonType, typename CompatibleObjectType>
struct is_compatible_object_type
    : is_compatible_object_type_impl<BasicJsonType, CompatibleObjectType> {};

export template<typename BasicJsonType, typename ConstructibleObjectType, typename = void>
struct is_constructible_object_type_impl: std::false_type {};

export template<typename BasicJsonType, typename ConstructibleObjectType>
struct is_constructible_object_type_impl<
        BasicJsonType,
        ConstructibleObjectType,
        enable_if_t<is_detected<mapped_type_t, ConstructibleObjectType>::value && is_detected<key_type_t, ConstructibleObjectType>::value>> {
    using object_t = typename BasicJsonType::object_t;

    static constexpr bool value =
            (is_default_constructible<ConstructibleObjectType>::value &&
             (std::is_move_assignable<ConstructibleObjectType>::value ||
              std::is_copy_assignable<ConstructibleObjectType>::value) &&
             (is_constructible<typename ConstructibleObjectType::key_type, typename object_t::key_type>::value &&
              std::is_same<
                      typename object_t::mapped_type,
                      typename ConstructibleObjectType::mapped_type>::value)) ||
            (has_from_json<BasicJsonType, typename ConstructibleObjectType::mapped_type>::value ||
             has_non_default_from_json<
                     BasicJsonType,
                     typename ConstructibleObjectType::mapped_type>::value);
};

export template<typename BasicJsonType, typename ConstructibleObjectType>
struct is_constructible_object_type
    : is_constructible_object_type_impl<BasicJsonType, ConstructibleObjectType> {};

export template<typename BasicJsonType, typename CompatibleStringType>
struct is_compatible_string_type {
    static constexpr auto value =
            is_constructible<typename BasicJsonType::string_t, CompatibleStringType>::value;
};

export template<typename BasicJsonType, typename ConstructibleStringType>
struct is_constructible_string_type {

#ifdef __INTEL_COMPILER
    using laundered_type = decltype(std::declval<ConstructibleStringType>());
#else
    using laundered_type = ConstructibleStringType;
#endif

    static constexpr auto value =
            conjunction<
                    is_constructible<laundered_type, typename BasicJsonType::string_t>,
                    is_detected_exact<typename BasicJsonType::string_t::value_type, value_type_t, laundered_type>>::value;
};

export template<typename BasicJsonType, typename CompatibleArrayType, typename = void>
struct is_compatible_array_type_impl: std::false_type {};

export template<typename BasicJsonType, typename CompatibleArrayType>
struct is_compatible_array_type_impl<
        BasicJsonType,
        CompatibleArrayType,
        enable_if_t<
                is_detected<iterator_t, CompatibleArrayType>::value &&
                is_iterator_traits<iterator_traits<detected_t<iterator_t, CompatibleArrayType>>>::value &&

                !std::is_same<CompatibleArrayType, detected_t<range_value_t, CompatibleArrayType>>::value>> {
    static constexpr bool value =
            is_constructible<BasicJsonType, range_value_t<CompatibleArrayType>>::value;
};

export template<typename BasicJsonType, typename CompatibleArrayType>
struct is_compatible_array_type
    : is_compatible_array_type_impl<BasicJsonType, CompatibleArrayType> {};

export template<typename BasicJsonType, typename ConstructibleArrayType, typename = void>
struct is_constructible_array_type_impl: std::false_type {};

export template<typename BasicJsonType, typename ConstructibleArrayType>
struct is_constructible_array_type_impl<
        BasicJsonType,
        ConstructibleArrayType,
        enable_if_t<std::is_same<ConstructibleArrayType, typename BasicJsonType::value_type>::value>>
    : std::true_type {};

export template<typename BasicJsonType, typename ConstructibleArrayType>
struct is_constructible_array_type_impl<
        BasicJsonType,
        ConstructibleArrayType,
        enable_if_t<!std::is_same<ConstructibleArrayType, typename BasicJsonType::value_type>::value && !is_compatible_string_type<BasicJsonType, ConstructibleArrayType>::value && is_default_constructible<ConstructibleArrayType>::value && (std::is_move_assignable<ConstructibleArrayType>::value || std::is_copy_assignable<ConstructibleArrayType>::value) && is_detected<iterator_t, ConstructibleArrayType>::value && is_iterator_traits<iterator_traits<detected_t<iterator_t, ConstructibleArrayType>>>::value && is_detected<range_value_t, ConstructibleArrayType>::value &&

                    !std::is_same<ConstructibleArrayType, detected_t<range_value_t, ConstructibleArrayType>>::value && is_complete_type<detected_t<range_value_t, ConstructibleArrayType>>::value>> {
    using value_type = range_value_t<ConstructibleArrayType>;

    static constexpr bool value =
            std::is_same<value_type, typename BasicJsonType::array_t::value_type>::value ||
            has_from_json<BasicJsonType, value_type>::value ||
            has_non_default_from_json<
                    BasicJsonType,
                    value_type>::value;
};

export template<typename BasicJsonType, typename ConstructibleArrayType>
struct is_constructible_array_type
    : is_constructible_array_type_impl<BasicJsonType, ConstructibleArrayType> {};

export template<typename RealIntegerType, typename CompatibleNumberIntegerType, typename = void>
struct is_compatible_integer_type_impl: std::false_type {};

export template<typename RealIntegerType, typename CompatibleNumberIntegerType>
struct is_compatible_integer_type_impl<
        RealIntegerType,
        CompatibleNumberIntegerType,
        enable_if_t<std::is_integral<RealIntegerType>::value && std::is_integral<CompatibleNumberIntegerType>::value && !std::is_same<bool, CompatibleNumberIntegerType>::value>> {

    using RealLimits = std::numeric_limits<RealIntegerType>;
    using CompatibleLimits = std::numeric_limits<CompatibleNumberIntegerType>;

    static constexpr auto value =
            is_constructible<RealIntegerType, CompatibleNumberIntegerType>::value &&
            CompatibleLimits::is_integer &&
            RealLimits::is_signed == CompatibleLimits::is_signed;
};

export template<typename RealIntegerType, typename CompatibleNumberIntegerType>
struct is_compatible_integer_type
    : is_compatible_integer_type_impl<RealIntegerType, CompatibleNumberIntegerType> {};

export template<typename BasicJsonType, typename CompatibleType, typename = void>
struct is_compatible_type_impl: std::false_type {};

export template<typename BasicJsonType, typename CompatibleType>
struct is_compatible_type_impl<
        BasicJsonType,
        CompatibleType,
        enable_if_t<is_complete_type<CompatibleType>::value>> {
    static constexpr bool value =
            has_to_json<BasicJsonType, CompatibleType>::value;
};

export template<typename BasicJsonType, typename CompatibleType>
struct is_compatible_type
    : is_compatible_type_impl<BasicJsonType, CompatibleType> {};

export template<typename T1, typename T2>
struct is_constructible_tuple: std::false_type {};

export template<typename T1, typename... Args>
struct is_constructible_tuple<T1, std::tuple<Args...>>: conjunction<is_constructible<T1, Args>...> {};

export template<typename BasicJsonType, typename T>
struct is_json_iterator_of: std::false_type {};

export template<typename BasicJsonType>
struct is_json_iterator_of<BasicJsonType, typename BasicJsonType::iterator>: std::true_type {};

export template<typename BasicJsonType>
struct is_json_iterator_of<BasicJsonType, typename BasicJsonType::const_iterator>: std::true_type {};

export template<template<typename...> class Primary, typename T>
struct is_specialization_of: std::false_type {};

export template<template<typename...> class Primary, typename... Args>
struct is_specialization_of<Primary, Primary<Args...>>: std::true_type {};

export template<typename T>
using is_json_pointer = is_specialization_of<::silicon::json::impl::json_pointer, uncvref_t<T>>;

export template<typename Compare, typename A, typename B, typename = void>
struct is_comparable: std::false_type {};

export template<typename Compare, typename A, typename B>
struct is_comparable<Compare, A, B, void_t<decltype(std::declval<Compare>()(std::declval<A>(), std::declval<B>())), decltype(std::declval<Compare>()(std::declval<B>(), std::declval<A>()))>>: std::true_type {};

export template<typename T>
using detect_is_transparent = typename T::is_transparent;

export template<typename Comparator, typename ObjectKeyType, typename KeyTypeCVRef, bool RequireTransparentComparator = true, bool ExcludeObjectKeyType = RequireTransparentComparator, typename KeyType = uncvref_t<KeyTypeCVRef>>
using is_usable_as_key_type = typename std::conditional<
        is_comparable<Comparator, ObjectKeyType, KeyTypeCVRef>::value && !(ExcludeObjectKeyType && std::is_same<KeyType, ObjectKeyType>::value) && (!RequireTransparentComparator || is_detected<detect_is_transparent, Comparator>::value) && !is_json_pointer<KeyType>::value,
        std::true_type,
        std::false_type>::type;

export template<typename BasicJsonType, typename KeyTypeCVRef, bool RequireTransparentComparator = true, bool ExcludeObjectKeyType = RequireTransparentComparator, typename KeyType = uncvref_t<KeyTypeCVRef>>
using is_usable_as_basic_json_key_type = typename std::conditional<
        is_usable_as_key_type<typename BasicJsonType::object_comparator_t, typename BasicJsonType::object_t::key_type, KeyTypeCVRef, RequireTransparentComparator, ExcludeObjectKeyType>::value && !is_json_iterator_of<BasicJsonType, KeyType>::value,
        std::true_type,
        std::false_type>::type;

export template<typename ObjectType, typename KeyType>
using detect_erase_with_key_type = decltype(std::declval<ObjectType &>().erase(std::declval<KeyType>()));

export template<typename BasicJsonType, typename KeyType>
using has_erase_with_key_type = typename std::conditional<
        is_detected<
                detect_erase_with_key_type,
                typename BasicJsonType::object_t,
                KeyType>::value,
        std::true_type,
        std::false_type>::type;

export template<typename T>
struct is_ordered_map {
    using one = char;

    struct two {
        char x[2];
    };

    template<typename C>
    static one test(decltype(&C::capacity));
    template<typename C>
    static two test(...);

    enum { value = sizeof(test<T>(nullptr)) == sizeof(char) };
};

export template<typename T, typename U, enable_if_t<!std::is_same<T, U>::value, int> = 0>
T conditional_static_cast(U value) {
    return static_cast<T>(value);
}

export template<typename T, typename U, enable_if_t<std::is_same<T, U>::value, int> = 0>
T conditional_static_cast(U value) {
    return value;
}

export template<typename... Types>
using all_integral = conjunction<std::is_integral<Types>...>;

export template<typename... Types>
using all_signed = conjunction<std::is_signed<Types>...>;

export template<typename... Types>
using all_unsigned = conjunction<std::is_unsigned<Types>...>;

export template<typename... Types>
using same_sign = std::integral_constant<bool, all_signed<Types...>::value || all_unsigned<Types...>::value>;

export template<typename OfType, typename T>
using never_out_of_range = std::integral_constant<bool, (std::is_signed<OfType>::value && (sizeof(T) < sizeof(OfType))) || (same_sign<OfType, T>::value && sizeof(OfType) == sizeof(T))>;

export template<typename OfType, typename T, bool OfTypeSigned = std::is_signed<OfType>::value, bool TSigned = std::is_signed<T>::value>
struct value_in_range_of_impl2;

export template<typename OfType, typename T>
struct value_in_range_of_impl2<OfType, T, false, false> {
    static constexpr bool test(T val) {
        using CommonType = typename std::common_type<OfType, T>::type;
        return static_cast<CommonType>(val) <= static_cast<CommonType>((std::numeric_limits<OfType>::max)());
    }
};

export template<typename OfType, typename T>
struct value_in_range_of_impl2<OfType, T, true, false> {
    static constexpr bool test(T val) {
        using CommonType = typename std::common_type<OfType, T>::type;
        return static_cast<CommonType>(val) <= static_cast<CommonType>((std::numeric_limits<OfType>::max)());
    }
};

export template<typename OfType, typename T>
struct value_in_range_of_impl2<OfType, T, false, true> {
    static constexpr bool test(T val) {
        using CommonType = typename std::common_type<OfType, T>::type;
        return val >= 0 && static_cast<CommonType>(val) <= static_cast<CommonType>((std::numeric_limits<OfType>::max)());
    }
};

export template<typename OfType, typename T>
struct value_in_range_of_impl2<OfType, T, true, true> {
    static constexpr bool test(T val) {
        using CommonType = typename std::common_type<OfType, T>::type;
        return static_cast<CommonType>(val) >= static_cast<CommonType>((std::numeric_limits<OfType>::min)()) && static_cast<CommonType>(val) <= static_cast<CommonType>((std::numeric_limits<OfType>::max)());
    }
};

export template<typename OfType, typename T, bool NeverOutOfRange = never_out_of_range<OfType, T>::value, typename = detail::enable_if_t<all_integral<OfType, T>::value>>
struct value_in_range_of_impl1;

export template<typename OfType, typename T>
struct value_in_range_of_impl1<OfType, T, false> {
    static constexpr bool test(T val) {
        return value_in_range_of_impl2<OfType, T>::test(val);
    }
};

export template<typename OfType, typename T>
struct value_in_range_of_impl1<OfType, T, true> {
    static constexpr bool test(T ) {
        return true;
    }
};

export template<typename OfType, typename T>
inline constexpr bool value_in_range_of(T val) {
    return value_in_range_of_impl1<OfType, T>::test(val);
}

export template<bool Value>
using bool_constant = std::integral_constant<bool, Value>;

namespace impl {

export template<typename T>
inline constexpr bool is_c_string() {
    using TUnExt = typename std::remove_extent<T>::type;
    using TUnCVExt = typename std::remove_cv<TUnExt>::type;
    using TUnPtr = typename std::remove_pointer<T>::type;
    using TUnCVPtr = typename std::remove_cv<TUnPtr>::type;
    return (std::is_array<T>::value && std::is_same<TUnCVExt, char>::value) || (std::is_pointer<T>::value && std::is_same<TUnCVPtr, char>::value);
}

}

export template<typename T>
struct is_c_string: bool_constant<impl::is_c_string<T>()> {};

export template<typename T>
using is_c_string_uncvref = is_c_string<uncvref_t<T>>;

namespace impl {

export template<typename T>
inline constexpr bool is_transparent() {
    return is_detected<detect_is_transparent, T>::value;
}

}

export template<typename T>
struct is_transparent: bool_constant<impl::is_transparent<T>()> {};

}
SILICON_JSON_NAMESPACE_END

module;

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <cassert>
#include <exception>
#include <utility>
#include <cstddef>
#include <initializer_list>
#include <tuple>
#include <memory>
#include <optional>
#include <variant>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <limits>
#include <array>
#include <new>
#include <typeindex>
#include <atomic>
#include <unordered_map>

#include <expected>
#include <system_error>
#include <iostream>
#include <silicon/proxy/proxy_macros.h>

export module silicon.di:facade;
export import silicon.di.error;
import silicon.proxy;


// Logical functional partitioning of the single self-contained :facade
// partition. The di subdirectories form a strongly-connected include
// component, so C++20 + xmake require everything in ONE translation
// unit. These banners only annotate the original per-directory
// boundaries (DFS include order preserved) for maintainability.


// ==============================================================================
// ==  core  —  binding model, binding collection & context
// ==============================================================================

// --- core/config.h ---

#if !defined(SILICON_DI_CONSTRUCTOR_DETECTION_ARGS)
#define SILICON_DI_CONSTRUCTOR_DETECTION_ARGS 32
#endif

#if !defined(SILICON_DI_CLOSURE_ARENA_BUFFER_SIZE)
#define SILICON_DI_CLOSURE_ARENA_BUFFER_SIZE 128
#endif

#if !defined(SILICON_DI_CONTEXT_ARENA_BUFFER_SIZE)
#define SILICON_DI_CONTEXT_ARENA_BUFFER_SIZE 128
#endif

#if !defined(SILICON_DI_ALWAYS_INLINE)
#if defined(_MSC_VER)
#define SILICON_DI_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define SILICON_DI_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define SILICON_DI_ALWAYS_INLINE inline
#endif
#endif

#if __cplusplus > 202002L || (defined(_MSVC_LANG) && _MSVC_LANG > 202002L)
#define SILICON_DI_CXX_STANDARD 23
#elif (__cplusplus > 201703L && __cplusplus <= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG == 202002L)
#define SILICON_DI_CXX_STANDARD 20
#elif __cplusplus <= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG == 201703L)
#define SILICON_DI_CXX_STANDARD 17
#endif


// ==============================================================================
// ==  type  —  type traits, descriptors, type lists & maps
// ==============================================================================

// --- type/type_descriptor.h ---



export namespace silicon::di {

enum class type_cv_flags : std::uint8_t {
    kNone = 0,
    kIsConst = 1 << 0,
    kIsVolatile = 1 << 1,
};

enum class type_reference_kind : std::uint8_t {
    kNone = 0,
    kLvalue = 1,
    kRvalue = 2,
};

struct type_descriptor {
    std::string_view raw_name;
    type_cv_flags cv = type_cv_flags::kNone;
    type_reference_kind reference = type_reference_kind::kNone;
    type_descriptor (*pointee)() = nullptr;
};

constexpr bool operator==(type_descriptor lhs, type_descriptor rhs) {
    if (lhs.raw_name != rhs.raw_name || lhs.cv != rhs.cv ||
        lhs.reference != rhs.reference) {
        return false;
    }

    if (lhs.pointee == nullptr || rhs.pointee == nullptr) {
        return lhs.pointee == rhs.pointee;
    }

    return lhs.pointee() == rhs.pointee();
}

template <typename T> constexpr std::string_view raw_type_name();
template <typename T> constexpr type_descriptor describe_type();
inline void append_type_name(std::string&, type_descriptor);



constexpr size_t type_name_not_found = static_cast<size_t>(-1);

constexpr size_t type_name_find(std::string_view haystack,
                                std::string_view needle,
                                size_t offset = 0) {
    if (needle.size() == 0) {
        return offset <= haystack.size() ? offset : type_name_not_found;
    }

    for (size_t i = offset; i + needle.size() <= haystack.size(); ++i) {
        size_t j = 0;
        for (; j < needle.size(); ++j) {
            if (haystack[i + j] != needle[j]) {
                break;
            }
        }
        if (j == needle.size()) {
            return i;
        }
    }

    return type_name_not_found;
}

template <typename T> constexpr std::string_view wrapped_type_name() {
#if defined(__clang__) || defined(__GNUC__)
    return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    return __FUNCSIG__;
#else
    return "unsupported compiler";
#endif
}

template <typename T> constexpr std::string_view wrapped_type_name_prefix() {
#if defined(__clang__)
    return "[T = ";
#elif defined(__GNUC__)
    return "with T = ";
#elif defined(_MSC_VER)
    return "wrapped_type_name<";
#else
    return "";
#endif
}

template <typename T> constexpr std::string_view wrapped_type_name_suffix() {
#if defined(__clang__)
    return "]";
#elif defined(__GNUC__)
    return ";";
#elif defined(_MSC_VER)
    return ">(void)";
#else
    return "";
#endif
}

template <typename T> constexpr std::string_view raw_type_name() {
    constexpr auto wrapped = wrapped_type_name<T>();
    constexpr auto prefix = wrapped_type_name_prefix<T>();
    constexpr auto suffix = wrapped_type_name_suffix<T>();
    constexpr auto prefix_pos = type_name_find(wrapped, prefix);

    static_assert(prefix_pos != type_name_not_found,
                  "failed to parse compiler type name prefix");

    constexpr auto start = prefix_pos + prefix.size();
    constexpr auto end = type_name_find(wrapped, suffix, start);

    static_assert(end != type_name_not_found,
                  "failed to parse compiler type name suffix");

    return std::string_view(wrapped.data() + start, end - start);
}

constexpr type_cv_flags operator|(type_cv_flags lhs, type_cv_flags rhs) {
    return static_cast<type_cv_flags>(static_cast<std::uint8_t>(lhs) |
                                      static_cast<std::uint8_t>(rhs));
}

constexpr bool has_cv_flag(type_cv_flags flags, type_cv_flags flag) {
    return (static_cast<std::uint8_t>(flags) & static_cast<std::uint8_t>(flag)) !=
           0;
}

template <typename T> constexpr type_cv_flags make_type_cv_flags() {
    type_cv_flags flags = type_cv_flags::kNone;

    if constexpr (std::is_const_v<T>) {
        flags = flags | type_cv_flags::kIsConst;
    }

    if constexpr (std::is_volatile_v<T>) {
        flags = flags | type_cv_flags::kIsVolatile;
    }

    return flags;
}

template <typename T> constexpr type_reference_kind make_type_reference_kind() {
    if constexpr (std::is_lvalue_reference_v<T>) {
        return type_reference_kind::kLvalue;
    } else if constexpr (std::is_rvalue_reference_v<T>) {
        return type_reference_kind::kRvalue;
    } else {
        return type_reference_kind::kNone;
    }
}

template <typename T> constexpr type_descriptor make_type_descriptor() {
    if constexpr (std::is_pointer_v<T>) {
        return {{}, make_type_cv_flags<T>(), type_reference_kind::kNone,
                &make_type_descriptor<std::remove_pointer_t<T>>};
    } else {
        return {raw_type_name<std::remove_cv_t<T>>(), make_type_cv_flags<T>(),
                type_reference_kind::kNone, nullptr};
    }
}

inline void append_type_cv(std::string& name, type_cv_flags flags,
                           std::string_view separator = " ") {
    if (has_cv_flag(flags, type_cv_flags::kIsConst)) {
        name += separator;
        name += "const";
        separator = " ";
    }

    if (has_cv_flag(flags, type_cv_flags::kIsVolatile)) {
        name += separator;
        name += "volatile";
    }
}

inline void append_described_type_name(std::string& name,
                                       type_descriptor descriptor) {
    if (descriptor.pointee) {
        append_described_type_name(name, descriptor.pointee());
        name += "*";
        append_type_cv(name, descriptor.cv);
    } else {
        if (has_cv_flag(descriptor.cv, type_cv_flags::kIsConst)) {
            name += "const ";
        }

        if (has_cv_flag(descriptor.cv, type_cv_flags::kIsVolatile)) {
            name += "volatile ";
        }

        name += descriptor.raw_name;
    }

    if (descriptor.reference == type_reference_kind::kLvalue) {
        name += "&";
    } else if (descriptor.reference == type_reference_kind::kRvalue) {
        name += "&&";
    }
}



template <typename T> constexpr type_descriptor describe_type() {
    auto descriptor = make_type_descriptor<std::remove_reference_t<T>>();
    descriptor.reference = make_type_reference_kind<T>();
    return descriptor;
}

inline void append_type_name(std::string& name, type_descriptor descriptor) {
    append_described_type_name(name, descriptor);
}

} // export namespace silicon::di


// ==============================================================================
// ==  core  —  binding model, binding collection & context
// ==============================================================================

export namespace silicon::di {

inline void append_text_part(std::string& message, type_descriptor descriptor) {
    append_type_name(message, descriptor);
}

template <typename Text>
void append_text_part(std::string& message, Text&& text) {
    message += std::forward<Text>(text);
}

inline void append_text(std::string&) {}

template <typename First, typename... Rest>
void append_text(std::string& message, First&& first, Rest&&... rest) {
    append_text_part(message, std::forward<First>(first));
    append_text(message, std::forward<Rest>(rest)...);
}

template <typename Request>
std::error_code make_type_not_found_exception() {
    return make_error_code(di_error::kTypeNotFound);
}

template <typename Request, typename Context>
std::error_code make_type_not_found_exception(const Context& context) {
    return make_error_code(di_error::kTypeNotFound);
}

template <typename Request, typename IdType>
std::error_code make_type_not_found_exception() {
    return make_error_code(di_error::kTypeNotFound);
}

template <typename Request, typename IdType, typename Context>
std::error_code make_type_not_found_exception(const Context& context) {
    return make_error_code(di_error::kTypeNotFound);
}

template <typename Collection, typename ResolveType>
std::error_code make_collection_type_not_found_exception() {
    return make_error_code(di_error::kCollectionTypeNotFound);
}

template <typename Request>
std::error_code make_type_ambiguous_exception() {
    return make_error_code(di_error::kTypeAmbiguous);
}

template <typename Request, typename Context>
std::error_code make_type_ambiguous_exception(const Context& context) {
    return make_error_code(di_error::kTypeAmbiguous);
}

inline std::error_code make_type_not_convertible_exception(
    type_descriptor target_type, type_descriptor source_type) {
    return make_error_code(di_error::kTypeNotConvertible);
}

template <typename Context>
inline std::error_code make_type_not_convertible_exception(
    type_descriptor target_type, type_descriptor source_type,
    const Context& context) {
    return make_error_code(di_error::kTypeNotConvertible);
}

template <typename Type> std::error_code make_type_recursion_exception() {
    return make_error_code(di_error::kTypeRecursion);
}

template <typename Type, typename Context>
std::error_code make_type_recursion_exception(const Context& context) {
    return make_error_code(di_error::kTypeRecursion);
}

template <typename Interface, typename Storage>
std::error_code make_type_already_registered_exception() {
    return make_error_code(di_error::kTypeAlreadyRegistered);
}

template <typename Interface, typename Storage, typename IdType>
std::error_code
make_type_index_already_registered_exception() {
    return make_error_code(di_error::kTypeIndexAlreadyRegistered);
}

template <typename Key>
std::error_code make_type_index_out_of_range_exception(
    Key, size_t) {
    return make_error_code(di_error::kIndexOutOfRange);
}


} // export namespace silicon::di


// ==============================================================================
// ==  registration  —  type registration, annotations & requirements
// ==============================================================================

// --- registration/annotated.h ---



export namespace silicon::di {

template <typename T, typename Tag, bool IsConstructible = std::is_constructible_v<T> > struct annotated_base;

template <typename T, typename Tag> struct annotated_base<T, Tag, true> {
    annotated_base(const annotated_base&) = delete;
    annotated_base(annotated_base&&) = delete;

    annotated_base(T&& value): value_(std::move(value)) {}
    operator T() { return std::move(value_); }
private:
    T value_;
};

template <typename T, typename Tag> struct annotated_base<T&, Tag, false> {
    annotated_base(const annotated_base&) = delete;
    annotated_base(annotated_base&&) = delete;

    annotated_base(T& value): value_(value) {}
    operator T&() { return value_; }
private:
    T& value_;
};

template <typename T, typename Tag> struct annotated_base<T, Tag, false>: annotated_base<T&, Tag, false> {};

template <typename T, typename Tag> struct annotated: annotated_base<T, Tag> {
    annotated(T&& value): annotated_base<T, Tag>(std::move(value)) {}
};

template <typename T, typename Tag> struct annotated<T&, Tag>: annotated_base<T&, Tag> {
    annotated(T& value): annotated_base<T&, Tag>(value) {}
};

template <typename T>
struct annotated_traits {
    using type = T;
};

template <typename T, typename Tag>
struct annotated_traits<annotated<T, Tag>> {
    using type = T;
};

template <typename T, typename Tag>
struct annotated_traits<annotated<T, Tag>&> {
    using type = T&;
};

template <typename T, typename Tag>
struct annotated_traits<annotated<T, Tag>&&> {
    using type = T&&;
};

template <typename T, typename Tag>
struct annotated_traits<annotated<T, Tag>*> {
    using type = T*;
};

} // export namespace silicon::di


// ==============================================================================
// ==  core  —  binding model, binding collection & context
// ==============================================================================

// --- core/keyed.h ---



export namespace silicon::di {
template <typename T> struct key;


template <typename T, typename Key, bool IsConstructible = std::is_constructible_v<T>>
struct keyed_base;

template <typename T, typename Key>
struct keyed_base<T, Key, true> {
    keyed_base(const keyed_base&) = default;
    keyed_base(keyed_base&&) = default;
    keyed_base& operator=(const keyed_base&) = default;
    keyed_base& operator=(keyed_base&&) = default;

    explicit keyed_base(T&& value)
        : value_(std::move(value)) {}

    operator T() { return std::move(value_); }

  private:
    T value_;
};

template <typename T, typename Key>
struct keyed_base<T&, Key, false> {
    keyed_base(const keyed_base&) = default;
    keyed_base(keyed_base&&) = default;
    keyed_base& operator=(const keyed_base&) = default;
    keyed_base& operator=(keyed_base&&) = default;

    explicit keyed_base(T& value)
        : value_(value) {}

    operator T&() { return value_; }

  private:
    T& value_;
};

template <typename T, typename Key>
struct keyed_base<T, Key, false> : keyed_base<T&, Key, false> {
    explicit keyed_base(T& value)
        : keyed_base<T&, Key, false>(value) {}
};


template <typename T, typename Key>
struct keyed : keyed_base<T, Key> {
    using keyed_base<T, Key>::keyed_base;
};

template <typename T, typename Key>
struct keyed<T&, Key> : keyed_base<T&, Key> {
    using keyed_base<T&, Key>::keyed_base;
};

template <typename T>
struct keyed_traits {
    using type = T;
    using key_type = void;
};

template <typename T, typename Key>
struct keyed_traits<keyed<T, Key>> {
    using type = T;
    using key_type = Key;
};

template <typename T, typename Key>
struct keyed_traits<keyed<T, Key>&> {
    using type = T&;
    using key_type = Key;
};

template <typename T, typename Key>
struct keyed_traits<keyed<T, Key>&&> {
    using type = T&&;
    using key_type = Key;
};

template <typename T, typename Key>
struct keyed_traits<keyed<T, Key>*> {
    using type = T*;
    using key_type = Key;
};

template <typename T>
using keyed_type_t = typename keyed_traits<T>::type;

template <typename T>
using keyed_key_t = typename keyed_traits<T>::key_type;

template <typename T>
struct is_keyed : std::bool_constant<!std::is_void_v<keyed_key_t<T>>> {};

template <typename T>
inline constexpr bool is_keyed_v = is_keyed<T>::value;



template <typename T> struct is_typed_key : std::false_type {};

template <typename T>
struct is_typed_key<key<T>> : std::bool_constant<!std::is_void_v<T>> {};

template <typename T>
inline constexpr bool is_typed_key_v = is_typed_key<std::decay_t<T>>::value;

template <typename Identity, typename Binding>
struct keyed_binding_identity : Binding {
    using Binding::Binding;
};



} // export namespace silicon::di


// ==============================================================================
// ==  type  —  type traits, descriptors, type lists & maps
// ==============================================================================

// --- type/type_list.h ---



export namespace silicon::di {
template <typename... Types> struct type_list {};
template <typename T> struct type_list_iterator {
    using type = T;
};


template <typename Accumulated, typename... Lists> struct type_list_cat_impl;

template <typename... Accumulated>
struct type_list_cat_impl<type_list<Accumulated...>> {
    using type = type_list<Accumulated...>;
};

template <typename... Accumulated, typename... Head, typename... Tail>
struct type_list_cat_impl<type_list<Accumulated...>, type_list<Head...>,
                          Tail...>
    : type_list_cat_impl<type_list<Accumulated..., Head...>, Tail...> {
};


template <typename... Lists> struct type_list_cat {
    using type = typename type_list_cat_impl<type_list<>, Lists...>::type;
};

template <typename... Lists>
using type_list_cat_t = typename type_list_cat<Lists...>::type;

template <typename List> struct type_list_size;

template <typename... Types>
struct type_list_size<type_list<Types...>>
    : std::integral_constant<size_t, sizeof...(Types)> {};

template <typename List>
inline constexpr size_t type_list_size_v = type_list_size<List>::value;

template <typename List> struct type_list_head;

template <typename Head, typename... Tail>
struct type_list_head<type_list<Head, Tail...>> {
    using type = Head;
};

template <typename List>
using type_list_head_t = typename type_list_head<List>::type;

template <typename T, typename List> struct type_list_contains;

template <typename T>
struct type_list_contains<T, type_list<>> : std::false_type {};

template <typename T, typename Head, typename... Tail>
struct type_list_contains<T, type_list<Head, Tail...>>
    : std::bool_constant<std::is_same_v<T, Head> ||
                         type_list_contains<T, type_list<Tail...>>::value> {};

template <typename T, typename List>
inline constexpr bool type_list_contains_v = type_list_contains<T, List>::value;


template <typename Accumulated, typename Remaining>
struct type_list_unique_impl;

template <typename... Accumulated>
struct type_list_unique_impl<type_list<Accumulated...>, type_list<>> {
    using type = type_list<Accumulated...>;
};

template <typename... Accumulated, typename Head, typename... Tail>
struct type_list_unique_impl<type_list<Accumulated...>,
                             type_list<Head, Tail...>> {
  private:
    using next_accumulated = std::conditional_t<
        type_list_contains_v<Head, type_list<Accumulated...>>,
        type_list<Accumulated...>, type_list<Accumulated..., Head>>;

  public:
    using type = typename type_list_unique_impl<next_accumulated,
                                                type_list<Tail...>>::type;
};


template <typename List>
using type_list_unique_t =
    typename type_list_unique_impl<type_list<>, List>::type;

template <typename T> struct to_type_list {
    using type = T;
};

template <typename... Types> struct to_type_list<std::tuple<Types...>> {
    using type = type_list<typename to_type_list<Types>::type...>;
};

template <typename T>
using to_type_list_t = typename to_type_list<T>::type;

template <typename RTTI, typename Function, typename... Types>
bool for_type(type_list<Types...>, const typename RTTI::type_index& type,
              Function&& fn) {
    bool matched = false;
    (void)std::initializer_list<int>{
        (((!matched && RTTI::template get_type_index<Types>() == type)
              ? (fn(type_list_iterator<Types>{}), matched = true, 0)
              : 0))...};
    return matched;
}

template <typename Function, typename... Types>
void for_each(type_list<Types...>, Function&& fn) {
    (fn(type_list_iterator<Types>{}), ...);
}

} // export namespace silicon::di

// --- type/type_traits.h ---




export namespace silicon::di {
struct unique;
struct shared;
struct external;

template <class T> struct exact_lookup;
template <class T> struct collection_traits;

template <typename T, typename = void> struct type_traits {
    static constexpr bool enabled = false;
    static constexpr bool is_pointer_like = false;
    static constexpr bool is_value_borrowable = false;

    template <typename>
    static constexpr bool is_handle_rebindable = false;

    template <typename>
    static constexpr bool is_rebindable = false;
};
template <typename T>
inline constexpr bool is_pointer_like_type_v =
    type_traits<T>::enabled && type_traits<T>::is_pointer_like &&
    !std::is_pointer_v<T>;

template <typename T, typename = void> struct copy_constructible_traits;


template <typename T, bool IsCollection>
struct copy_constructible_with_collection {
    static constexpr bool value = std::is_copy_constructible_v<T>;
};

template <typename T> struct copy_constructible_with_collection<T, true> {
    static constexpr bool value =
        copy_constructible_traits<
            typename collection_traits<T>::resolve_type>::value;
};

template <typename T, typename = void> struct copy_constructible_base {
    using value_type = std::remove_cv_t<std::remove_reference_t<T>>;

    static constexpr bool value = std::is_copy_constructible_v<value_type>;
};

template <typename T>
struct copy_constructible_base<
    T, std::void_t<decltype(collection_traits<std::remove_cv_t<
                             std::remove_reference_t<T>>>::is_collection)>> {
    using collection_type =
        collection_traits<std::remove_cv_t<std::remove_reference_t<T>>>;
    using value_type = std::remove_cv_t<std::remove_reference_t<T>>;

    static constexpr bool value =
        copy_constructible_with_collection<
            value_type, collection_type::is_collection>::value;
};


template <typename T, typename>
struct copy_constructible_traits : copy_constructible_base<T> {};

template <typename T>
inline constexpr bool is_copy_constructible_v =
    copy_constructible_traits<T>::value;

template <typename Type, typename Selected, typename = void>
struct construction_traits {
    static constexpr bool enabled = false;
};

template <typename T, typename = void> struct alternative_type_traits {
    static constexpr bool enabled = false;
};

template <typename T>
inline constexpr bool is_alternative_type_v =
    alternative_type_traits<std::remove_cv_t<T>>::enabled;


template <typename List, typename Selected> struct type_list_count;

template <typename Selected, typename... Alternatives>
struct type_list_count<type_list<Alternatives...>, Selected>
    : std::integral_constant<size_t,
                             (0u + ... + (std::is_same_v<Selected, Alternatives> ? 1u
                                                                                 : 0u))> {};

template <typename List> struct type_list_has_duplicates;

template <typename... Alternatives>
struct type_list_has_duplicates<type_list<Alternatives...>>
    : std::bool_constant<
          ((type_list_count<type_list<Alternatives...>, Alternatives>::value > 1) ||
           ...)> {};

template <typename Type, typename = void>
struct alternative_type_alternatives {};

template <typename Type>
struct alternative_type_alternatives<
    Type,
    std::enable_if_t<alternative_type_traits<std::remove_cv_t<Type>>::enabled>> {
    using type = typename alternative_type_traits<std::remove_cv_t<Type>>::alternatives;

    static_assert(
        !type_list_has_duplicates<type>::value,
        "alternative_type_traits<T>::alternatives must not contain duplicate types");
};

template <typename Type>
using alternative_type_alternatives_t =
    typename alternative_type_alternatives<Type>::type;

template <typename Type, typename Selected, typename = void>
struct alternative_type_count : std::integral_constant<size_t, 0> {};

template <typename Type, typename Selected>
struct alternative_type_count<
    Type, Selected,
    std::enable_if_t<alternative_type_traits<std::remove_cv_t<Type>>::enabled>>
    : type_list_count<alternative_type_alternatives_t<Type>,
                      std::remove_cv_t<Selected>> {};

template <typename Type, typename = void>
struct alternative_type_interface_types {};

template <typename Type>
struct alternative_type_interface_types<
    Type,
    std::enable_if_t<alternative_type_traits<std::remove_cv_t<Type>>::enabled>> {
    using type =
        type_list_cat_t<type_list<std::remove_cv_t<Type>>,
                        alternative_type_alternatives_t<Type>>;
};


template <typename... Alternatives>
struct alternative_type_traits<std::variant<Alternatives...>> {
    static constexpr bool enabled = true;

    using alternatives = type_list<Alternatives...>;

    template <typename Selected, typename Value>
    static std::variant<Alternatives...> wrap(Value&& value) {
        return std::variant<Alternatives...>(std::in_place_type<Selected>,
                                             std::forward<Value>(value));
    }

    template <typename Selected>
    static Selected* get(std::variant<Alternatives...>& value) {
        return std::get_if<Selected>(&value);
    }

    template <typename Selected>
    static const Selected* get(const std::variant<Alternatives...>& value) {
        return std::get_if<Selected>(&value);
    }
};

template <typename Type, typename Interface>
inline constexpr bool is_alternative_type_interface_compatible_v =
    alternative_type_count<std::remove_cv_t<Type>,
                                   std::remove_cv_t<Interface>>::value == 1;

template <typename Type, typename Selected>
struct construction_traits<
    Type, Selected,
    std::enable_if_t<is_alternative_type_v<Type> &&
                     (alternative_type_count<Type, Selected>::value ==
                      1)>> {
    static constexpr bool enabled = true;

    using type = std::remove_cv_t<Type>;

    template <typename Value> static type wrap(Value&& value) {
        return alternative_type_traits<type>::template wrap<Selected>(
            std::forward<Value>(value));
    }
};

template <typename Type, typename Selected>
struct construction_traits<
    Type, Selected,
    std::enable_if_t<type_traits<Type>::enabled && !std::is_pointer_v<Type> &&
                     construction_traits<typename type_traits<Type>::value_type,
                                         Selected>::enabled>> {
    static constexpr bool enabled = true;

    using value_type = typename type_traits<Type>::value_type;
    using type = Type;

    template <typename Value> static type wrap(Value&& value) {
        return type_traits<Type>::make(
            construction_traits<value_type, Selected>::wrap(
                std::forward<Value>(value)));
    }
};

template <typename StorageTag, typename Type, typename U, typename = void>
struct storage_traits {
    static constexpr bool enabled = false;
    static constexpr bool is_stable = false;

    using value_types = type_list<>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<>;
    using conversion_types = type_list<>;
};

template <typename StorageTag, typename Type, typename U>
struct storage_traits<StorageTag, const Type, U>
    : storage_traits<StorageTag, Type, U> {};

template <typename StorageTag, typename Type, typename U>
struct storage_traits<StorageTag, Type&, U> : storage_traits<StorageTag, Type, U> {};

template <typename StorageTag, typename Type, typename U>
struct storage_traits<StorageTag, const Type&, U>
    : storage_traits<StorageTag, Type, U> {};


template <typename T, typename... Args> T make_nested(Args&&... args);

template <typename T, size_t N, typename... Args>
T* make_bounded_array(Args&&... args) {
    static_assert(sizeof...(Args) <= N,
                  "too many initializers for bounded array construction");
    return new T[N]{std::forward<Args>(args)...};
}

template <typename T, size_t N, typename... Args>
void construct_bounded_array(void* ptr, Args&&... args) {
    static_assert(sizeof...(Args) <= N,
                  "too many initializers for bounded array construction");
    new (ptr) T[N]{std::forward<Args>(args)...};
}

template <typename T, typename... Args> T* make_dynamic_array(Args&&... args) {
    static_assert(sizeof...(Args) > 0,
                  "dynamic arrays require an explicit size via a custom "
                  "factory or element initializers");
    return new T[sizeof...(Args)]{std::forward<Args>(args)...};
}

template <typename Type, typename Pointer, typename = void>
struct has_type_from_pointer : std::false_type {};

template <typename Type, typename Pointer>
struct has_type_from_pointer<
    Type, Pointer,
    std::void_t<decltype(type_traits<Type>::from_pointer(
        std::declval<Pointer>()))>> : std::true_type {};

template <typename Type, typename Pointer>
inline constexpr bool has_type_from_pointer_v =
    has_type_from_pointer<Type, Pointer>::value;

template <typename Type, typename = void>
struct is_array_like_type : std::is_array<Type> {};

template <typename Type>
inline constexpr bool is_array_like_type_v = is_array_like_type<Type>::value;

template <typename Type, typename = void>
struct array_like_exact_interface_type {
    using type = Type;
};

template <typename Type>
using array_like_exact_interface_type_t =
    typename array_like_exact_interface_type<Type>::type;

template <typename Type, typename U, typename = void>
struct wrapper_rebind_leaf {
    using type = U;
};

template <typename Type, size_t N, typename U>
struct wrapper_rebind_leaf<Type[N], U, void> {
    using type = typename wrapper_rebind_leaf<Type, U>::type[N];
};

template <typename Type, typename U>
struct wrapper_rebind_leaf<Type[], U, void> {
    using type = typename wrapper_rebind_leaf<Type, U>::type[];
};

template <typename Type, typename U>
struct wrapper_rebind_leaf<
    Type, U,
    std::enable_if_t<type_traits<Type>::enabled && !std::is_pointer_v<Type>>> {
    using type = typename type_traits<Type>::template rebind_t<
        typename wrapper_rebind_leaf<typename type_traits<Type>::value_type,
                                     U>::type>;
};

template <typename Type, typename U>
using wrapper_rebind_leaf_t = typename wrapper_rebind_leaf<Type, U>::type;

template <typename Handle, typename T, typename U, typename = void>
struct smart_array_pointer_types {
    using type = type_list<U*, Handle*>;
};

template <typename Handle, typename T, typename U>
struct smart_array_pointer_types<
    Handle, T, U, std::enable_if_t<std::is_array_v<T>>> {
    using type =
        type_list<exact_lookup<typename wrapper_rebind_leaf<T, U>::type>*,
                  Handle*>;
};

template <typename Handle, typename = void>
struct wrapper_storage_types_impl {
    using lvalue_reference_types = type_list<Handle&>;
    using pointer_types = type_list<Handle*>;
    using copyable_value_types = std::conditional_t<
        std::is_copy_constructible_v<Handle>, type_list<Handle>, type_list<>>;
};

template <typename Handle>
struct wrapper_storage_types_impl<
    Handle, std::enable_if_t<type_traits<Handle>::enabled &&
                             type_traits<Handle>::is_pointer_like>> {
  private:
    // Walk the wrapper chain once and collect all derived interface types
    // together instead of recursing over the same chain separately for each
    // result list.
    using next =
        wrapper_storage_types_impl<typename type_traits<Handle>::value_type>;
    using copyable_handle_types = std::conditional_t<
        std::is_copy_constructible_v<Handle>, type_list<Handle>, type_list<>>;

  public:
    using lvalue_reference_types =
        type_list_cat_t<type_list<Handle&>, typename next::lvalue_reference_types>;
    using pointer_types =
        type_list_cat_t<type_list<Handle*>, typename next::pointer_types>;
    using copyable_value_types = type_list_cat_t<
        copyable_handle_types, typename next::copyable_value_types>;
};

template <typename Handle>
using wrapper_storage_types = wrapper_storage_types_impl<Handle>;

template <typename Array, typename Deleter>
struct is_array_like_type<
    std::unique_ptr<Array, Deleter>,
    std::enable_if_t<std::is_array_v<Array>>> : std::true_type {};

template <typename Array, typename Deleter>
struct array_like_exact_interface_type<
    std::unique_ptr<Array, Deleter>,
    std::enable_if_t<std::is_array_v<Array>>> {
    using type = typename type_traits<std::unique_ptr<Array, Deleter>>::value_type;
};

template <typename Array>
struct is_array_like_type<std::shared_ptr<Array>,
                          std::enable_if_t<std::is_array_v<Array>>>
    : std::true_type {};

template <typename Array>
struct array_like_exact_interface_type<
    std::shared_ptr<Array>, std::enable_if_t<std::is_array_v<Array>>> {
    using type = typename type_traits<std::shared_ptr<Array>>::value_type;
};



template <typename T> struct type_traits<T*> {
    static constexpr bool enabled = true;
    static constexpr bool is_pointer_like = true;
    static constexpr bool is_value_borrowable = true;

    template <typename Target>
    static constexpr bool is_handle_rebindable =
        std::is_constructible_v<Target, T*>;

    using value_type = T;

    template <typename U> using rebind_t = U*;

    template <typename Target>
    static constexpr bool is_rebindable = std::is_same_v<Target, rebind_t<T>>;

    static T* get(T* ptr) { return ptr; }
    static T& borrow(T* ptr) { return *ptr; }
    static bool empty(T* ptr) { return ptr == nullptr; }
    static void reset(T*& ptr) { ptr = nullptr; }
    static T* from_pointer(T* ptr) { return ptr; }

    template <typename TargetType, typename Factory, typename Context>
    static std::expected<TargetType, std::error_code> resolve_type(
            Factory& factory, Context& context,
            type_descriptor requested_type,
            type_descriptor registered_type) {
        if constexpr (std::is_same_v<TargetType, rebind_t<T>>)
            return factory.resolve(context);
        else
            return std::unexpected(make_type_not_convertible_exception(
                    requested_type, registered_type, context));
    }
};

template <> struct type_traits<void*> {
    static constexpr bool enabled = true;
    static constexpr bool is_pointer_like = true;
    static constexpr bool is_value_borrowable = false;

    template <typename Target>
    static constexpr bool is_handle_rebindable =
        std::is_constructible_v<Target, void*>;

    using value_type = void;

    template <typename U> using rebind_t = U*;

    template <typename Target>
    static constexpr bool is_rebindable =
        std::is_same_v<Target, rebind_t<void>>;

    static void* get(void* ptr) { return ptr; }
    static bool empty(void* ptr) { return ptr == nullptr; }
    static void reset(void*& ptr) { ptr = nullptr; }
    static void* from_pointer(void* ptr) { return ptr; }

    template <typename TargetType, typename Factory, typename Context>
    static std::expected<TargetType, std::error_code> resolve_type(
            Factory& factory, Context& context,
            type_descriptor requested_type,
            type_descriptor registered_type) {
        if constexpr (std::is_same_v<TargetType, rebind_t<void>>)
            return factory.resolve(context);
        else
            return std::unexpected(make_type_not_convertible_exception(
                    requested_type, registered_type, context));
    }
};

template <> struct type_traits<const void*> {
    static constexpr bool enabled = true;
    static constexpr bool is_pointer_like = true;
    static constexpr bool is_value_borrowable = false;

    template <typename Target>
    static constexpr bool is_handle_rebindable =
        std::is_constructible_v<Target, const void*>;

    using value_type = const void;

    template <typename U> using rebind_t = U*;

    template <typename Target>
    static constexpr bool is_rebindable =
        std::is_same_v<Target, rebind_t<const void>>;

    static const void* get(const void* ptr) { return ptr; }
    static bool empty(const void* ptr) { return ptr == nullptr; }
    static void reset(const void*& ptr) { ptr = nullptr; }
    static const void* from_pointer(const void* ptr) { return ptr; }

    template <typename TargetType, typename Factory, typename Context>
    static std::expected<TargetType, std::error_code> resolve_type(
            Factory& factory, Context& context,
            type_descriptor requested_type,
            type_descriptor registered_type) {
        if constexpr (std::is_same_v<TargetType, rebind_t<const void>>)
            return factory.resolve(context);
        else
            return std::unexpected(make_type_not_convertible_exception(
                    requested_type, registered_type, context));
    }
};

template <typename Array, typename Deleter>
struct type_traits<
    std::unique_ptr<Array, Deleter>,
    std::enable_if_t<std::is_array_v<Array> && (std::extent_v<Array, 0> == 0)>> {
    static constexpr bool enabled = true;
    static constexpr bool is_pointer_like = true;
    static constexpr bool is_value_borrowable = false;

    template <typename Target>
    static constexpr bool is_handle_rebindable =
        std::is_constructible_v<Target, std::unique_ptr<Array, Deleter>>;

    using value_type = std::remove_extent_t<Array>;

    template <typename U>
    using rebind_t = std::unique_ptr<U[], std::default_delete<U[]>>;

    template <typename Target>
    static constexpr bool is_rebindable =
        std::is_same_v<Target, rebind_t<value_type>>;

    static value_type* get(std::unique_ptr<Array, Deleter>& wrapper) {
        return wrapper.get();
    }

    static const value_type* get(const std::unique_ptr<Array, Deleter>& wrapper) {
        return wrapper.get();
    }

    static bool empty(const std::unique_ptr<Array, Deleter>& wrapper) {
        return wrapper.get() == nullptr;
    }

    static void reset(std::unique_ptr<Array, Deleter>& wrapper) {
        wrapper.reset();
    }

    template <typename... Args>
    static std::unique_ptr<Array, Deleter> make(Args&&... args) {
        return std::unique_ptr<Array, Deleter>(make_dynamic_array<value_type>(
            std::forward<Args>(args)...));
    }

    static std::unique_ptr<Array, Deleter> from_pointer(value_type* ptr) {
        return std::unique_ptr<Array, Deleter>(ptr);
    }

    template <typename TargetType, typename Factory, typename Context>
    static std::expected<std::reference_wrapper<TargetType>, std::error_code>
    resolve_type(Factory& factory, Context& context,
                 type_descriptor requested_type,
                 type_descriptor registered_type) {
        if constexpr (std::is_same_v<TargetType, rebind_t<value_type>>)
            return factory.resolve(context);
        else
            return std::unexpected(make_type_not_convertible_exception(
                    requested_type, registered_type, context));
    }
};

template <typename T, typename Deleter>
struct type_traits<std::unique_ptr<T, Deleter>,
                   std::enable_if_t<!std::is_array_v<T>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_pointer_like = true;
    static constexpr bool is_value_borrowable = true;

    template <typename Target>
    static constexpr bool is_handle_rebindable =
        std::is_constructible_v<Target, std::unique_ptr<T, Deleter>>;

    using value_type = T;

    template <typename U>
    using rebind_t = std::unique_ptr<U, std::default_delete<U>>;

    template <typename Target>
    static constexpr bool is_rebindable =
        std::is_same_v<Target, rebind_t<T>>;

    static T* get(std::unique_ptr<T, Deleter>& wrapper) {
        return wrapper.get();
    }

    static const T* get(const std::unique_ptr<T, Deleter>& wrapper) {
        return wrapper.get();
    }

    static T& borrow(std::unique_ptr<T, Deleter>& wrapper) {
        return *wrapper;
    }

    static const T& borrow(const std::unique_ptr<T, Deleter>& wrapper) {
        return *wrapper;
    }

    static bool empty(const std::unique_ptr<T, Deleter>& wrapper) {
        return wrapper.get() == nullptr;
    }

    static void reset(std::unique_ptr<T, Deleter>& wrapper) { wrapper.reset(); }

    template <typename... Args>
    static std::unique_ptr<T, Deleter> make(Args&&... args) {
        if constexpr (type_traits<T>::enabled && !std::is_pointer_v<T>)
            return std::unique_ptr<T, Deleter>(
                new T(make_nested<T>(std::forward<Args>(args)...)));
        // Work around direct-initialization in smart-pointer-backed factories.
        else if constexpr (std::is_constructible_v<T, Args...>)
            return std::unique_ptr<T, Deleter>(
                new T(std::forward<Args>(args)...));
        else
            return std::unique_ptr<T, Deleter>(
                new T{std::forward<Args>(args)...});
    }

    static std::unique_ptr<T, Deleter> from_pointer(T* ptr) {
        return std::unique_ptr<T, Deleter>(ptr);
    }

    template <typename TargetType, typename Factory, typename Context>
    static std::expected<std::reference_wrapper<TargetType>, std::error_code>
    resolve_type(Factory& factory, Context& context,
                 type_descriptor requested_type,
                 type_descriptor registered_type) {
        if constexpr (std::is_same_v<TargetType, rebind_t<T>>)
            return factory.resolve(context);
        else
            return std::unexpected(make_type_not_convertible_exception(
                    requested_type, registered_type, context));
    }
};


template <typename Array>
struct type_traits<
    std::shared_ptr<Array>,
    std::enable_if_t<std::is_array_v<Array> && (std::extent_v<Array, 0> == 0)>> {
    static constexpr bool enabled = true;
    static constexpr bool is_pointer_like = true;
    static constexpr bool is_value_borrowable = false;

    template <typename Target>
    static constexpr bool is_handle_rebindable =
        std::is_constructible_v<Target, std::shared_ptr<Array>>;

    using value_type = std::remove_extent_t<Array>;

    template <typename U> using rebind_t = std::shared_ptr<U[]>;

    template <typename Target>
    static constexpr bool is_rebindable =
        std::is_constructible_v<std::remove_cv_t<Target>, std::shared_ptr<Array>>;

    static value_type* get(std::shared_ptr<Array>& wrapper) { return wrapper.get(); }

    static const value_type* get(const std::shared_ptr<Array>& wrapper) {
        return wrapper.get();
    }

    static bool empty(const std::shared_ptr<Array>& wrapper) {
        return wrapper.get() == nullptr;
    }

    static void reset(std::shared_ptr<Array>& wrapper) { wrapper.reset(); }

    template <typename... Args> static std::shared_ptr<Array> make(Args&&... args) {
        return std::shared_ptr<Array>(make_dynamic_array<value_type>(
            std::forward<Args>(args)...));
    }

    static std::shared_ptr<Array> from_pointer(value_type* ptr) {
        return std::shared_ptr<Array>(ptr);
    }

    template <typename TargetType, typename Factory, typename Context>
    static TargetType& resolve_type(Factory& factory, Context& context,
                                    type_descriptor, type_descriptor) {
        return factory.template resolve<TargetType>(context);
    }
};

template <typename T>
struct type_traits<std::shared_ptr<T>, std::enable_if_t<!std::is_array_v<T>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_pointer_like = true;
    static constexpr bool is_value_borrowable = true;

    template <typename Target>
    static constexpr bool is_handle_rebindable =
        std::is_constructible_v<Target, std::shared_ptr<T>>;

    using value_type = T;

    template <typename U> using rebind_t = std::shared_ptr<U>;

    template <typename Target>
    static constexpr bool is_rebindable =
        std::is_constructible_v<std::remove_cv_t<Target>, std::shared_ptr<T>>;

    static T* get(std::shared_ptr<T>& wrapper) { return wrapper.get(); }

    static const T* get(const std::shared_ptr<T>& wrapper) {
        return wrapper.get();
    }

    static T& borrow(std::shared_ptr<T>& wrapper) { return *wrapper; }

    static const T& borrow(const std::shared_ptr<T>& wrapper) {
        return *wrapper;
    }

    static bool empty(const std::shared_ptr<T>& wrapper) {
        return wrapper.get() == nullptr;
    }

    static void reset(std::shared_ptr<T>& wrapper) { wrapper.reset(); }

    template <typename... Args> static std::shared_ptr<T> make(Args&&... args) {
        if constexpr (type_traits<T>::enabled && !std::is_pointer_v<T>)
            return std::make_shared<T>(
                make_nested<T>(std::forward<Args>(args)...));
        // Work around direct-initialization in std::make_shared().
        else if constexpr (std::is_constructible_v<T, Args...>)
            return std::make_shared<T>(std::forward<Args>(args)...);
        else
            return std::shared_ptr<T>(new T{std::forward<Args>(args)...});
    }

    static std::shared_ptr<T> from_pointer(T* ptr) {
        return std::shared_ptr<T>(ptr);
    }

    template <typename TargetType, typename Factory, typename Context>
    static TargetType& resolve_type(Factory& factory, Context& context,
                                    type_descriptor, type_descriptor) {
        return factory.template resolve<TargetType>(context);
    }
};


template <typename T> struct type_traits<std::optional<T>> {
    static constexpr bool enabled = true;
    static constexpr bool is_pointer_like = false;
    static constexpr bool is_value_borrowable = true;

    template <typename>
    static constexpr bool is_handle_rebindable = false;

    using value_type = T;

    template <typename U> using rebind_t = std::optional<U>;

    template <typename Target>
    static constexpr bool is_rebindable =
        std::is_same_v<Target, rebind_t<T>>;

    static T* get(std::optional<T>& wrapper) {
        return wrapper ? std::addressof(wrapper.value()) : nullptr;
    }

    static const T* get(const std::optional<T>& wrapper) {
        return wrapper ? std::addressof(wrapper.value()) : nullptr;
    }

    static T& borrow(std::optional<T>& wrapper) { return wrapper.value(); }

    static const T& borrow(const std::optional<T>& wrapper) {
        return wrapper.value();
    }

    static bool empty(const std::optional<T>& wrapper) {
        return !wrapper.has_value();
    }

    static void reset(std::optional<T>& wrapper) { wrapper.reset(); }

    template <typename... Args> static std::optional<T> make(Args&&... args) {
        if constexpr (type_traits<T>::enabled && !std::is_pointer_v<T>)
            return std::optional<T>{std::in_place,
                                    make_nested<T>(
                                        std::forward<Args>(args)...)};
        else if constexpr (std::is_constructible_v<T, Args...>)
            return std::optional<T>{std::in_place, std::forward<Args>(args)...};
        else
            return std::optional<T>{std::in_place,
                                    T{std::forward<Args>(args)...}};
    }

    template <typename TargetType, typename Factory, typename Context>
    static std::expected<std::reference_wrapper<TargetType>, std::error_code>
    resolve_type(Factory& factory, Context& context,
                 type_descriptor requested_type,
                 type_descriptor registered_type) {
        if constexpr (std::is_same_v<TargetType, rebind_t<T>>)
            return factory.resolve(context);
        else
            return std::unexpected(make_type_not_convertible_exception(
                    requested_type, registered_type, context));
    }
};


template <typename T, typename... Args> T make_nested(Args&&... args) {
    return type_traits<T>::make(std::forward<Args>(args)...);
}


} // export namespace silicon::di

// --- type/normalized_type.h ---




export namespace silicon::di {
template <class T, class = void> struct normalized_type : std::decay<T> {};

template <class T> struct normalized_type<const T> : normalized_type<T> {};
template <class T, size_t N>
struct normalized_type<T (*)[N], void> {
    using type = T[N];
};
template <class T, size_t N>
struct normalized_type<const T (*)[N], void> {
    using type = T[N];
};
template <class T, size_t N>
struct normalized_type<T (&)[N], void> {
    using type = T[N];
};
template <class T, size_t N>
struct normalized_type<const T (&)[N], void> {
    using type = T[N];
};
template <class T, size_t N> struct normalized_type<T[N]> : normalized_type<T> {};
template <class T> struct normalized_type<T[]> : normalized_type<T> {};
template <class T> struct normalized_type<T*> : normalized_type<T> {};
template <class T> struct normalized_type<const T*> : normalized_type<T> {};
template <class T> struct normalized_type<T&> : normalized_type<T> {};
template <class T> struct normalized_type<const T&> : normalized_type<T> {};
template <class T> struct normalized_type<T&&> : normalized_type<T> {};

template <class T>
struct normalized_type<
    T, std::enable_if_t<type_traits<T>::enabled && !std::is_pointer_v<T>>>
    : normalized_type<typename type_traits<T>::value_type> {};

template <class T, class Tag>
struct normalized_type<annotated<T, Tag>, void>
    : std::decay<annotated<typename normalized_type<T>::type, Tag>> {};

template <class T, class Key>
struct normalized_type<keyed<T, Key>, void>
    : std::decay<keyed<typename normalized_type<T>::type, Key>> {};

template <class T> using normalized_type_t = typename normalized_type<T>::type;
} // export namespace silicon::di


// ==============================================================================
// ==  registration  —  type registration, annotations & requirements
// ==============================================================================

// --- registration/collection_traits.h ---




export namespace silicon::di {

template <class T> struct collection_traits {
    static const bool is_collection = false;
    static const bool has_fixed_size_construct = false;
};

template <class T, class Allocator>
struct collection_traits<std::vector<T, Allocator>> {
    static const bool is_collection = true;
    static const bool has_fixed_size_construct = true;
    using resolve_type = T;
    static void reserve(std::vector<T, Allocator>& collection, size_t size) {
        collection.reserve(size);
    }
    static std::vector<T, Allocator> make_fixed_size(size_t size) {
        return std::vector<T, Allocator>(size);
    }
    template <typename U>
    static void add(std::vector<T, Allocator>& collection, U&& value) {
        collection.emplace_back(std::forward<U>(value));
    }
    template <typename U>
    static void set(std::vector<T, Allocator>& collection, size_t index,
                    U&& value) {
        collection[index] = std::forward<U>(value);
    }
};

template <class T, class Allocator>
struct collection_traits<std::list<T, Allocator>> {
    static const bool is_collection = true;
    static const bool has_fixed_size_construct = false;
    using resolve_type = T;
    static void reserve(std::list<T, Allocator>&, size_t) {}
    template <typename U>
    static void add(std::list<T, Allocator>& collection, U&& value) {
        collection.emplace_back(std::forward<U>(value));
    }
};

template <class T, class Compare, class Allocator>
struct collection_traits<std::set<T, Compare, Allocator>> {
    static const bool is_collection = true;
    static const bool has_fixed_size_construct = false;
    using resolve_type = T;
    static void reserve(std::set<T, Compare, Allocator>&, size_t) {}
    template <typename U>
    static void add(std::set<T, Compare, Allocator>& collection, U&& value) {
        collection.emplace(std::forward<U>(value));
    }
};

template <class Key, class Value, class Compare, class Allocator>
struct collection_traits<std::map<Key, Value, Compare, Allocator>> {
    static const bool is_collection = true;
    static const bool has_fixed_size_construct = false;
    using resolve_type = Value;
    static void reserve(std::map<Key, Value, Compare, Allocator>&, size_t) {}
    template <typename U>
    static void add(std::map<Key, Value, Compare, Allocator>& collection,
                    U&& value) {
        collection.emplace(std::forward<U>(value));
    }
};



} // export namespace silicon::di


// ==============================================================================
// ==  core  —  binding model, binding collection & context
// ==============================================================================

// --- core/binding_collection.h ---



export namespace silicon::di {

struct binding_collection_append {
    template <typename Collection, typename Value>
    void operator()(Collection& collection, Value&& value) const {
        collection_traits<std::decay_t<Collection>>::add(
            collection, std::forward<Value>(value));
    }
};

template <typename StaticRegistry, typename T, typename Key = void>
constexpr std::size_t static_collection_binding_count() {
    return type_list_size_v<typename StaticRegistry::template bindings<
        normalized_type_t<typename collection_traits<T>::resolve_type>, Key>>;
}

template <typename T, typename RuntimeCountFn>
std::size_t count_binding_collection(RuntimeCountFn&& runtime_count,
                                     std::size_t static_count) {
    return std::forward<RuntimeCountFn>(runtime_count)() + static_count;
}

template <typename T, typename RuntimeAppendFn, typename StaticAppendFn,
          typename Fn>
std::size_t append_binding_collection(T& results,
                                      RuntimeAppendFn&& runtime_append,
                                      StaticAppendFn&& static_append, Fn&& fn) {
    std::size_t count = 0;
    count += std::forward<RuntimeAppendFn>(runtime_append)(results, fn);
    count += std::forward<StaticAppendFn>(static_append)(results, fn);
    return count;
}

// --- expected 返回类型封装 ---
// di 的 resolve 入口可能返回 T&（引用），而 std::expected<T&, E> 标准不允许；
// 用 as_expected_t 把引用统一包成 reference_wrapper<T>，其余类型原样包入 expected。
template <typename T>
using as_expected_t = std::expected<
    std::conditional_t<std::is_reference_v<T>,
                       std::reference_wrapper<std::remove_reference_t<T>>,
                       T>,
    std::error_code>;
template <typename T, typename PrimaryCountFn, typename SecondaryCountFn,
          typename PrimaryAppendFn, typename SecondaryAppendFn, typename Fn>
as_expected_t<T> construct_binding_collection(PrimaryCountFn&& primary_count,
                               SecondaryCountFn&& secondary_count,
                               PrimaryAppendFn&& primary_append,
                               SecondaryAppendFn&& secondary_append, Fn&& fn) {
    using collection_type = collection_traits<T>;
    using resolve_type = typename collection_type::resolve_type;

    static_assert(collection_type::is_collection,
                  "missing collection_traits specialization for type T");

    const std::size_t total = std::forward<PrimaryCountFn>(primary_count)() +
                              std::forward<SecondaryCountFn>(secondary_count)();
    if (total == 0) {
        return std::unexpected(
            make_collection_type_not_found_exception<T, resolve_type>());
    }

    T results;
    collection_type::reserve(results, total);

    auto&& append = fn;
    std::forward<PrimaryAppendFn>(primary_append)(results, append);
    std::forward<SecondaryAppendFn>(secondary_append)(results, append);
    return results;
}

} // export namespace silicon::di


// ==============================================================================
// ==  type  —  type traits, descriptors, type lists & maps
// ==============================================================================

// --- type/complete_type.h ---


export namespace silicon::di {

// Detect completeness through overload resolution instead of a partial
// specialization. MSVC x64 accepts the specialization-based probe for some
// forward declarations that still need to be rejected during registration.
template <typename T, std::size_t = sizeof(T)>
std::true_type is_complete_impl(int);

template <typename T>
std::false_type is_complete_impl(...);

template <typename T>
struct is_complete : decltype(is_complete_impl<T>(0)) {};

template <typename T>
inline constexpr bool is_complete_v = is_complete<T>::value;

template <typename T>
inline constexpr bool requires_complete_type_v =
    !std::is_void_v<T> && !std::is_function_v<T>;

} // export namespace silicon::di


// ==============================================================================
// ==  core  —  binding model, binding collection & context
// ==============================================================================

// --- core/auto_constructible.h ---



export namespace silicon::di {


template <typename T, bool = is_complete<T>::value>
struct default_auto_constructible : std::false_type {};

template <typename T>
struct default_auto_constructible<T, true>
    : std::bool_constant<std::is_aggregate_v<T>> {};



template <typename T>
struct is_auto_constructible : default_auto_constructible<T> {};

} // export namespace silicon::di


// ==============================================================================
// ==  type  —  type traits, descriptors, type lists & maps
// ==============================================================================

// --- type/rebind_type.h ---




export namespace silicon::di {
struct runtime_type {};
template <class T> struct exact_lookup {
    using type = T;
};
template <class T, class = void> struct leaf_type;
template <class T, class U, class = void> struct rebind_type;
template <class T, class U, class = void> struct rebind_leaf_type;


template <class T> struct decoration_traits {
    using type = T;

    template <class U>
    using rebind_t = U;
};

template <class T> struct decoration_traits<const T> {
    using type = typename decoration_traits<T>::type;

    template <class U>
    using rebind_t = typename decoration_traits<T>::template rebind_t<U>;
};

template <class T> struct decoration_traits<T&> {
    using type = typename decoration_traits<T>::type;

    template <class U>
    using rebind_t = typename decoration_traits<T>::template rebind_t<U>&;
};

template <class T> struct decoration_traits<T&&> {
    using type = typename decoration_traits<T>::type;

    template <class U>
    using rebind_t = typename decoration_traits<T>::template rebind_t<U>&&;
};

template <class T> struct pointer_traits {
    using type = T;

    template <class U>
    using rebind_t = U;
};

template <class T> struct pointer_traits<T*> {
    using type = typename pointer_traits<T>::type;

    template <class U>
    using rebind_t = typename pointer_traits<T>::template rebind_t<U>*;
};

template <class T> struct pointer_traits<const T*> {
    using type = typename pointer_traits<T>::type;

    template <class U>
    using rebind_t = typename pointer_traits<T>::template rebind_t<U>*;
};

template <class T> struct outer_traits {
    using decoration = decoration_traits<T>;
    using pointer = pointer_traits<typename decoration::type>;
    using type = typename pointer::type;

    template <class U>
    using rebind_t = typename decoration::template rebind_t<
        typename pointer::template rebind_t<U>>;
};

template <class T, class = void> struct leaf_base {
    using type = T;
};

template <class T>
struct leaf_base<
    T,
    std::enable_if_t<type_traits<T>::enabled && !std::is_pointer_v<T>>> {
    using type = typename leaf_type<typename type_traits<T>::value_type>::type;
};

template <typename T, size_t N> struct leaf_base<T[N], void> {
    using type = typename leaf_type<T>::type;
};

template <typename T> struct leaf_base<T[], void> {
    using type = typename leaf_type<T>::type;
};

template <typename... Args> struct leaf_base<type_list<Args...>, void> {
    using type = type_list<typename leaf_type<Args>::type...>;
};

template <class T, class U, class = void> struct rebind_base {
    using type = U;
};

template <class T, class U>
struct rebind_base<
    T, U,
    std::enable_if_t<type_traits<T>::enabled && !std::is_pointer_v<T>>> {
    using type = typename type_traits<T>::template rebind_t<U>;
};

template <typename T, size_t N, class U> struct rebind_base<T[N], U, void> {
    using rebound = typename rebind_type<T, U>::type;
    using type = rebound[N];
};

template <typename T, class U> struct rebind_base<T[], U, void> {
    using rebound = typename rebind_type<T, U>::type;
    using type = rebound[];
};

template <typename U, typename... Args>
struct rebind_base<type_list<Args...>, U, void> {
    using type = type_list<typename rebind_type<Args, U>::type...>;
};

template <class T, class U, class = void> struct rebind_leaf_base {
    using type = typename leaf_type<U>::type;
};

template <class T, class U> struct rebind_leaf_base<exact_lookup<T>, U, void> {
    using type = T;
};

template <class T, class U>
struct rebind_leaf_base<
    T, U,
    std::enable_if_t<type_traits<T>::enabled && !std::is_pointer_v<T>>> {
    using type = typename type_traits<T>::template rebind_t<
        typename rebind_leaf_type<typename type_traits<T>::value_type, U>::type>;
};

template <typename T, size_t N, class U>
struct rebind_leaf_base<T[N], U, void> {
    using rebound = typename rebind_leaf_type<T, U>::type;
    using type = rebound[N];
};

template <typename T, class U> struct rebind_leaf_base<T[], U, void> {
    using rebound = typename rebind_leaf_type<T, U>::type;
    using type = rebound[];
};

template <typename U, typename... Args>
struct rebind_leaf_base<type_list<Args...>, U, void> {
    using type = type_list<typename rebind_leaf_type<Args, U>::type...>;
};

template <class T, class U, class = void> struct lookup_base {
    using type = typename rebind_leaf_base<T, runtime_type>::type;
};

template <class T, class U>
struct lookup_base<exact_lookup<T>, U, void> {
    using type = typename rebind_leaf_type<T, runtime_type>::type;
};

template <class T, class U, class = void> struct resolved_base {
    using type = typename rebind_leaf_base<T, U>::type;
};

template <class T, class U>
struct resolved_base<
    exact_lookup<T>, U,
    std::enable_if_t<!std::is_same_v<typename leaf_type<T>::type,
                                     runtime_type>>> {
    using type = T;
};

template <class T, class U>
struct resolved_base<
    exact_lookup<T>, U,
    std::enable_if_t<std::is_same_v<typename leaf_type<T>::type,
                                    runtime_type>>> {
    using type = typename rebind_leaf_type<T, typename leaf_type<U>::type>::type;
};


template <class T, class> struct leaf_type {
  private:
    using outer = outer_traits<T>;

  public:
    using type = typename leaf_base<typename outer::type>::type;
};

template <class T> using leaf_type_t = typename leaf_type<T>::type;

template <class T, class U, class> struct rebind_type {
  private:
    using outer = outer_traits<T>;
    using rebound = typename rebind_base<typename outer::type, U>::type;

  public:
    using type = typename outer::template rebind_t<rebound>;
};

template <typename T, typename U>
using rebind_type_t = typename rebind_type<T, U>::type;

template <class T, class U, class> struct rebind_leaf_type {
  private:
    using outer = outer_traits<T>;
    using rebound = typename rebind_leaf_base<typename outer::type, U>::type;

  public:
    using type = typename outer::template rebind_t<rebound>;
};

template <typename T, typename U>
using rebind_leaf_t = typename rebind_leaf_type<T, U>::type;

template <class T, class = void> struct lookup_type {
  private:
    using outer = outer_traits<T>;
    using rebound = typename lookup_base<typename outer::type, runtime_type>::type;

  public:
    using type = typename outer::template rebind_t<rebound>;
};

template <typename T>
using lookup_type_t = typename lookup_type<T>::type;

template <class T, class U, class = void> struct resolved_type {
  private:
    using outer = outer_traits<T>;
    using rebound = typename resolved_base<typename outer::type, U>::type;

  public:
    using type = typename outer::template rebind_t<rebound>;
};

template <typename T, typename U>
using resolved_type_t = typename resolved_type<T, U>::type;

template <class T> struct is_exact_lookup : std::false_type {};
template <class T> struct is_exact_lookup<exact_lookup<T>> : std::true_type {};
template <class T> struct is_exact_lookup<const T> : is_exact_lookup<T> {};
template <class T> struct is_exact_lookup<T&> : is_exact_lookup<T> {};
template <class T> struct is_exact_lookup<T&&> : is_exact_lookup<T> {};
template <class T> struct is_exact_lookup<T*> : is_exact_lookup<T> {};
template <class T> struct is_exact_lookup<const T*> : is_exact_lookup<T> {};

template <class T>
inline constexpr bool is_exact_lookup_v = is_exact_lookup<T>::value;

} // export namespace silicon::di


// ==============================================================================
// ==  storage  —  storage policies (shared / unique / external / cyclical)
// ==============================================================================

// --- storage/interface_storage_traits.h ---




export namespace silicon::di {

template <typename Source, typename Target>
inline constexpr bool is_handle_rebindable_v =
    type_traits<Source>::enabled && type_traits<Source>::is_pointer_like &&
    type_traits<Source>::template is_handle_rebindable<Target>;


template <typename Storage, typename Interface>
inline constexpr bool is_interface_storage_rebindable_v =
    is_handle_rebindable_v<
        Storage, rebind_leaf_t<Storage,
                               typename annotated_traits<Interface>::type>>;


template <typename Storage, typename InterfaceList>
struct use_interface_as_stored_leaf;

template <typename Storage, typename... Interfaces>
struct use_interface_as_stored_leaf<Storage, type_list<Interfaces...>> : std::bool_constant<false> {};

template <typename Storage, typename Interface>
inline constexpr bool can_store_interface_as_leaf_v = [] {
    using interface_type = typename annotated_traits<Interface>::type;
    if constexpr (!requires_complete_type_v<interface_type>) {
        return false;
    } else if constexpr (!is_complete_v<interface_type>) {
        return false;
    } else if constexpr (!std::is_class_v<interface_type>) {
        return false;
    } else {
        return std::has_virtual_destructor_v<interface_type> &&
               is_interface_storage_rebindable_v<Storage, Interface>;
    }
}();

template <typename Storage, typename Interface>
struct use_interface_as_stored_leaf<Storage, type_list<Interface>>
    : std::bool_constant<can_store_interface_as_leaf_v<Storage, Interface>> {};

template <typename Storage, typename InterfaceList>
inline constexpr bool use_interface_as_stored_leaf_v =
    use_interface_as_stored_leaf<Storage, InterfaceList>::value;

} // export namespace silicon::di


// ==============================================================================
// ==  factory  —  constructor detection, callable & function injection
// ==============================================================================

// --- factory/constructor_traits.h ---



#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

export namespace silicon::di {

template <typename T, typename = void> struct constructor_traits {
    template <typename... Args> static T construct(Args&&... args) {
        return T{std::forward<Args>(args)...};
    }

    template <typename... Args>
    static void construct(void* ptr, Args&&... args) {
        new (ptr) T{std::forward<Args>(args)...};
    }
};

template <typename T> struct constructor_traits<T*> {
    template <typename... Args> static T* construct(Args&&... args) {
        return new T{std::forward<Args>(args)...};
    }

    template <typename... Args>
    static void construct(void* ptr, Args&&... args) {
        new (ptr) T{std::forward<Args>(args)...};
    }
};

template <typename T, size_t N> struct constructor_traits<T[N]> {
    template <typename... Args> static T* construct(Args&&... args) {
        return make_bounded_array<T, N>(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void construct(void* ptr, Args&&... args) {
        construct_bounded_array<T, N>(ptr,
                                              std::forward<Args>(args)...);
    }
};

template <typename T> struct constructor_traits<T&> {
    template <typename... Args> static T& construct(Args&&...) {
        static_assert(true, "references cannot be constructed");
    }
};

template <typename T>
struct constructor_traits<
    T, std::enable_if_t<type_traits<T>::enabled && !std::is_pointer_v<T>>> {
    template <typename... Args> static T construct(Args&&... args) {
        return type_traits<T>::make(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static T& construct(T& ptr, Args&&... args) {
        ptr = type_traits<T>::make(std::forward<Args>(args)...);
        return ptr;
    }

    template <typename... Args>
    static void construct(void* ptr, Args&&... args) {
        new (ptr) T(type_traits<T>::make(std::forward<Args>(args)...));
    }
};


template <typename Type, typename Selected, typename = void>
struct construction_dispatch {
    template <typename... Args> static auto construct(Args&&... args) {
        return constructor_traits<Type>::construct(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void construct(void* ptr, Args&&... args) {
        constructor_traits<Type>::construct(ptr, std::forward<Args>(args)...);
    }
};

template <typename Type, typename Selected>
struct construction_dispatch<
    Type, Selected,
    std::enable_if_t<construction_traits<Type, Selected>::enabled>> {
    using type = typename construction_traits<Type, Selected>::type;

    template <typename... Args> static auto construct(Args&&... args) {
        return construction_traits<Type, Selected>::wrap(
            constructor_traits<Selected>::construct(std::forward<Args>(args)...));
    }

    template <typename... Args>
    static void construct(void* ptr, Args&&... args) {
        new (ptr) type(construction_traits<Type, Selected>::wrap(
            constructor_traits<Selected>::construct(std::forward<Args>(args)...)));
    }
};

template <typename Type, typename Selected>
struct construction_dispatch<
    Type*, Selected,
    std::enable_if_t<construction_traits<Type, Selected>::enabled>> {
    using type = typename construction_traits<Type, Selected>::type;

    template <typename... Args> static auto construct(Args&&... args) {
        return new type(construction_traits<Type, Selected>::wrap(
            constructor_traits<Selected>::construct(std::forward<Args>(args)...)));
    }

    template <typename... Args>
    static void construct(void* ptr, Args&&... args) {
        new (ptr) type(construction_traits<Type, Selected>::wrap(
            constructor_traits<Selected>::construct(std::forward<Args>(args)...)));
    }
};


} // export namespace silicon::di

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// --- factory/constructor_typedef.h ---



export namespace silicon::di {

// Keep typedef detection in this header lightweight so opt-in construction
// annotations can include it without pulling in the heavier construction
// backend machinery from constructor_traits.h.
template <typename T, typename = void>
struct has_constructor_typedef : std::false_type {};

template <typename T>
struct has_constructor_typedef<
    T, typename std::void_t<typename T::di_constructor_type>>
    : std::true_type {};

template <typename T>
inline constexpr bool has_constructor_typedef_v =
    has_constructor_typedef<T>{};


enum class constructor_kind { kConcrete, kGeneric, kInvalid };

template <typename T, bool = has_constructor_typedef_v<T>>
struct constructor_typedef_impl : T::di_constructor_type {};

template <typename T> struct constructor_typedef_impl<T, false> {};


template <typename T>
struct constructor_typedef : constructor_typedef_impl<T> {
    static constexpr constructor_kind kind =
        constructor_kind::kConcrete;
};

} // export namespace silicon::di

// --- factory/constructor_detection.h ---




export namespace silicon::di {

template <typename T> struct constructor_detection_traits {
    static constexpr size_t max_arity = SILICON_DI_CONSTRUCTOR_DETECTION_ARGS;
};


struct automatic {};

template <typename Detection, typename = void>
struct constructor_detection_arguments {
    using type = std::conditional_t<
        Detection::kind == constructor_kind::kConcrete && Detection::arity == 0,
        type_list<>, void>;
};

template <typename Detection>
struct constructor_detection_arguments<
    Detection, std::void_t<typename Detection::arguments>> {
    using type = typename Detection::arguments;
};

template <typename Detection>
using constructor_detection_arguments_t =
    typename constructor_detection_arguments<Detection>::type;

template <class DisabledType, typename Tag> struct constructor_argument;
template <class DisabledType, typename Tag> struct opaque_constructor_argument;

template <class DisabledType>
struct constructor_argument<DisabledType, automatic> {
    // Value and rvalue probes imply materialization, so they must not
    // participate for forward declarations. Otherwise constructor deduction
    // would pull incomplete dependencies into class-trait inspection.
    template <
        typename T,
        typename = typename std::enable_if_t<
            !std::is_same_v<DisabledType, std::decay_t<T>> &&
            is_complete<std::decay_t<T>>::value>
    >
    operator T&&() const;

    // Lvalue-reference probes stay available for incomplete types. They model
    // borrowed dependencies that can be satisfied by lookup without trying to
    // construct the forward-declared type.
    template <
        typename T,
        typename = typename std::enable_if_t< !std::is_same_v<DisabledType, std::decay_t<T>> >
    >
    operator const T&() const;

    template <
        typename T,
        typename = typename std::enable_if_t< !std::is_same_v<DisabledType, std::decay_t<T>> >
    >
    operator T&() const;

    template <
        typename T,
        typename = typename std::enable_if_t<
            !std::is_same_v<DisabledType, std::decay_t<T>> &&
            is_complete<std::decay_t<T>>::value>
    >
    operator T();

    template <
        typename T, typename Key,
        typename = typename std::enable_if_t<
            !std::is_same_v<DisabledType, std::decay_t<T>>>>
    operator keyed<T, Key>() const;
};

template <class DisabledType>
struct opaque_constructor_argument<DisabledType, automatic> {
    opaque_constructor_argument() = default;
    opaque_constructor_argument(const opaque_constructor_argument&) = default;
    opaque_constructor_argument(opaque_constructor_argument&&) = default;
    opaque_constructor_argument& operator=(const opaque_constructor_argument&) =
        default;
    opaque_constructor_argument& operator=(opaque_constructor_argument&&) =
        default;
};

template <typename DisabledType, typename Context, typename Container,
          typename Tag>
class constructor_argument_impl;

template <typename DisabledType, typename Context, typename Container>
class constructor_argument_impl<DisabledType, Context, Container, automatic> {
  public:
    constructor_argument_impl(Context& context, Container& container)
        : context_(context), container_(container) {}

    template <
        typename T,
        typename = typename std::enable_if_t< !std::is_same_v<DisabledType, std::decay_t<T>> >
    >
    operator const T&() const {
        return context_.template resolve<const T&>(container_);
    }

    template <
        typename T,
        typename = typename std::enable_if_t< !std::is_same_v<DisabledType, std::decay_t<T>> >
    >
    operator T&() const {
        return context_.template resolve<T&>(container_);
    }

    template <
        typename T,
        typename = typename std::enable_if_t<
            !std::is_same_v<DisabledType, std::decay_t<T>> &&
            is_complete<std::decay_t<T>>::value>
    >
    operator T() {
        // A prvalue `T` still satisfies `T&&` constructor parameters, so
        // constructor injection can stay on the regular value path here.
        return context_.template resolve<T>(container_);
    }

    template <typename T, typename Tag,
              typename = std::enable_if_t< !std::is_same_v<DisabledType, std::decay_t<T>>> >
    operator annotated<T, Tag>() {
        return context_.template resolve<annotated<T, Tag>>(container_);
    }

    template <typename T, typename Key,
              typename = std::enable_if_t<!std::is_same_v<DisabledType, std::decay_t<T>>>>
    operator keyed<T, Key>() {
        return context_.template resolve<keyed<T, Key>>(container_);
    }

  private:
    Context& context_;
    Container& container_;
};

template <typename T, typename... Args>
using list_initialization_expr = decltype(T{std::declval<Args>()...});

template <typename T, typename... Args>
using direct_initialization_expr = decltype(T(std::declval<Args>()...));

#if defined(_MSC_VER)
template <typename T, typename = void, typename... Args>
struct list_initialization_impl : std::false_type {};

template <typename T, typename... Args>
struct list_initialization_impl<
    T, std::void_t<decltype(T{std::declval<Args>()...})>, Args...>
    : std::true_type {};

template <typename T, typename... Args>
struct list_initialization : list_initialization_impl<T, void, Args...> {};

template <typename T, typename Arg>
struct list_initialization<T, Arg>
    : std::conjunction<list_initialization_impl<T, void, Arg>,
                       std::negation<std::is_same<std::decay_t<Arg>, T>>> {};

template <typename T, typename... Args>
inline constexpr bool is_list_initializable_v =
    list_initialization<T, Args...>::value;

template <typename T, typename = void, typename... Args>
struct direct_initialization_impl : std::false_type {};

template <typename T, typename... Args>
struct direct_initialization_impl<
    T, std::void_t<decltype(T(std::declval<Args>()...))>, Args...>
    : std::true_type {};

template <typename T, typename... Args>
struct direct_initialization : direct_initialization_impl<T, void, Args...> {};

template <typename T, typename Arg>
struct direct_initialization<T, Arg>
    : std::conjunction<direct_initialization_impl<T, void, Arg>,
                       std::negation<std::is_same<std::decay_t<Arg>, T>>> {};
#else
// Detection should not treat `T(T&)` as a meaningful dependency-taking
// constructor. Filtering the copy-construction shape here keeps both list and
// direct initialization probes aligned.
template <typename T, typename... Args>
inline constexpr bool is_non_copy_constructor_argument_v =
    sizeof...(Args) != 1 || (!std::is_same_v<T, std::decay_t<Args>> && ...);

template <template <typename, typename...> typename InitExpr, typename T,
          typename = void, typename... Args>
struct initialization_impl : std::false_type {};

template <template <typename, typename...> typename InitExpr, typename T,
          typename... Args>
struct initialization_impl<InitExpr, T, std::void_t<InitExpr<T, Args...>>,
                           Args...>
    : std::bool_constant<is_non_copy_constructor_argument_v<T, Args...>> {};

template <typename T, typename... Args>
struct list_initialization
    : initialization_impl<list_initialization_expr, T, void, Args...> {};

template <typename T, typename... Args>
inline constexpr bool is_list_initializable_v =
    list_initialization<T, Args...>::value;

template <typename T, typename... Args>
struct direct_initialization
    : initialization_impl<direct_initialization_expr, T, void, Args...> {};
#endif

template <typename T, typename... Args>
inline constexpr bool is_direct_initializable_v =
    direct_initialization<T, Args...>::value;

inline constexpr size_t invalid_constructor_detection_arity =
    static_cast<size_t>(-1);

template <typename...>
inline constexpr bool always_false_v = false;

template <typename T, size_t>
using repeated_type = T;

// Keep one shared detector/search shape and vary only the per-arity probe.
// Non-MSVC compilers handle a lighter constexpr probe well, while MSVC still
// needs the older type-based form to preserve behavior.
#if defined(_MSC_VER)
template <typename T, typename Tag,
          template <class, class> class ConstructorArg,
          template <typename...> typename IsConstructible, typename Sequence>
struct constructor_probe_msvc;

template <typename T, typename Tag,
          template <class, class> class ConstructorArg,
          template <typename...> typename IsConstructible, size_t... Is>
struct constructor_probe_msvc<T, Tag, ConstructorArg, IsConstructible,
                              std::index_sequence<Is...>>
    : IsConstructible<
          T,
          std::conditional_t<true, ConstructorArg<T, Tag>,
                             std::integral_constant<size_t, Is>>...> {};

template <typename T, typename Tag,
          template <class, class> class ConstructorArg,
          template <typename...> typename IsConstructible, size_t Arity>
inline constexpr bool constructor_probe_v =
    constructor_probe_msvc<T, Tag, ConstructorArg, IsConstructible,
                           std::make_index_sequence<Arity>>::value;
#else
template <typename T, typename Tag,
          template <class, class> class ConstructorArg,
          template <typename...> typename IsConstructible, size_t... Is>
constexpr bool constructor_probe(std::index_sequence<Is...>) {
    // Non-MSVC compilers handle the lighter repeated-type placeholder probe
    // well, which avoids an extra wrapper class per arity check.
    return IsConstructible<T, repeated_type<ConstructorArg<T, Tag>, Is>...>::value;
}

template <typename T, typename Tag,
          template <class, class> class ConstructorArg,
          template <typename...> typename IsConstructible, size_t Arity>
inline constexpr bool constructor_probe_v =
    constructor_probe<T, Tag, ConstructorArg, IsConstructible>(
        std::make_index_sequence<Arity>{});
#endif

// Everything above feeds the same high-to-low arity search below, so the
// selected constructor semantics stay shared even though the probe body differs
// by compiler.
template <typename T, typename Tag,
          template <typename...> typename IsConstructible, size_t Arity,
          bool Match = constructor_probe_v<T, Tag, constructor_argument,
                                           IsConstructible, Arity>>
struct constructor_arity_detector_impl
    : constructor_arity_detector_impl<T, Tag, IsConstructible, Arity - 1> {};

template <typename T, typename Tag,
          template <typename...> typename IsConstructible, size_t Arity>
struct constructor_arity_detector_impl<T, Tag, IsConstructible, Arity, true>
    : std::integral_constant<size_t, Arity> {};

template <typename T, typename Tag,
          template <typename...> typename IsConstructible>
struct constructor_arity_detector_impl<T, Tag, IsConstructible, 0, false>
    : std::integral_constant<size_t, invalid_constructor_detection_arity> {};

template <typename T, typename Tag,
          template <typename...> typename IsConstructible>
struct constructor_arity_detector_impl<T, Tag, IsConstructible, 0, true>
    : std::integral_constant<size_t, 0> {};

template <typename T, typename Tag,
          template <typename...> typename IsConstructible, size_t Arity>
using constructor_arity_detector =
    constructor_arity_detector_impl<T, Tag, IsConstructible, Arity>;

// Searches constructor arity in the inclusive range [0, N].
template <typename T, typename Tag, template <typename...> typename IsConstructible,
          size_t N = SILICON_DI_CONSTRUCTOR_DETECTION_ARGS>
struct constructor_detection_impl;

template <typename T, typename Tag = automatic>
using default_constructor_detection =
    constructor_detection_impl<T, Tag, list_initialization,
                          constructor_detection_traits<
                              normalized_type_t<T>>::max_arity>;

template <typename T, typename Tag, size_t Arity> struct constructor_methods {
  private:
    template <typename Type, typename Context, typename Container, size_t... Is>
    static auto construct_impl(Context& ctx, Container& container,
                               std::index_sequence<Is...>) {
        // `Is...` only drives the pack expansion; the runtime construction
        // path still receives `Arity` copies of the same constructor argument
        // adapter without first materializing a type_list of placeholders.
        return construction_dispatch<Type, T>::construct(
            ((void)Is, constructor_argument_impl<T, Context, Container, Tag>(
                           ctx, container))...);
    }

    template <typename Type, typename Context, typename Container, size_t... Is>
    static void construct_impl(void* ptr, Context& ctx, Container& container,
                               std::index_sequence<Is...>) {
        construction_dispatch<Type, T>::construct(
            ptr, ((void)Is,
                  constructor_argument_impl<T, Context, Container, Tag>(
                      ctx, container))...);
    }

  public:
    template <typename Type, typename Context, typename Container>
    static auto construct(Context& ctx, Container& container) {
        return construct_impl<Type>(ctx, container,
                                    std::make_index_sequence<Arity>{});
    }

    template <typename Type, typename Context, typename Container>
    static void construct(void* ptr, Context& ctx, Container& container) {
        construct_impl<Type>(ptr, ctx, container,
                             std::make_index_sequence<Arity>{});
    }
};

template <typename T, typename Tag, size_t Arity, constructor_kind Kind>
struct constructor_detection_dispatch;

template <typename T, typename Tag, size_t Arity>
struct constructor_detection_dispatch<T, Tag, Arity,
                                      constructor_kind::kConcrete> {
    template <typename Type, typename Context, typename Container>
    static auto construct(Context& ctx, Container& container) {
        return constructor_methods<T, Tag, Arity>::template construct<Type>(
            ctx, container);
    }

    template <typename Type, typename Context, typename Container>
    static void construct(void* ptr, Context& ctx, Container& container) {
        constructor_methods<T, Tag, Arity>::template construct<Type>(
            ptr, ctx, container);
    }
};

template <typename T, typename Tag, size_t Arity>
struct constructor_detection_dispatch<T, Tag, Arity,
                                      constructor_kind::kGeneric> {
    template <typename Type, typename Context, typename Container>
    static Type construct(Context&, Container&) {
        static_assert(
            always_false_v<Type>,
            "generic constructor detected; use explicit "
            "factory<constructor<...>>");
    }

    template <typename Type, typename Context, typename Container>
    static void construct(void*, Context&, Container&) {
        static_assert(
            always_false_v<Type>,
            "generic constructor detected; use explicit "
            "factory<constructor<...>>");
    }
};

template <typename T, typename Tag, size_t Arity>
struct constructor_detection_dispatch<T, Tag, Arity,
                                      constructor_kind::kInvalid> {
    template <typename Type, typename Context, typename Container>
    static Type construct(Context&, Container&) {
        static_assert(always_false_v<Type>,
                      "class T construction not detected or ambiguous");
    }

    template <typename Type, typename Context, typename Container>
    static void construct(void*, Context&, Container&) {
        static_assert(always_false_v<Type>,
                      "class T construction not detected or ambiguous");
    }
};

template <typename T, typename Tag, template <typename...> typename IsConstructible,
          size_t N>
struct constructor_detection_impl {
    // The detector owns policy: pick the highest matching arity once, then let
    // the runtime path instantiate only that winning constructor shape.
    // Search from high to low so the first match is the winning constructor
    // arity without materializing the full `[0, N]` probe set up front.
    static constexpr size_t detected_arity =
        constructor_arity_detector<T, Tag, IsConstructible, N>::value;
    static constexpr bool detected =
        detected_arity != invalid_constructor_detection_arity;
    static constexpr bool requires_explicit_factory = [] {
        if constexpr (!detected || detected_arity == 0) {
            return false;
        } else {
            return constructor_probe_v<T, Tag, opaque_constructor_argument,
                                       IsConstructible, detected_arity>;
        }
    }();
    static constexpr constructor_kind kind =
        !detected ? constructor_kind::kInvalid
                  : requires_explicit_factory ? constructor_kind::kGeneric
                                              : constructor_kind::kConcrete;
    static constexpr size_t arity = detected ? detected_arity : 0;
    using dispatch = constructor_detection_dispatch<T, Tag, arity, kind>;

  public:
    template <typename Type, typename Context, typename Container>
    static auto construct(Context& ctx, Container& container) {
        return dispatch::template construct<Type>(ctx, container);
    }

    template <typename Type, typename Context, typename Container>
    static void construct(void* ptr, Context& ctx, Container& container) {
        dispatch::template construct<Type>(ptr, ctx, container);
    }
};



template <typename T, typename DetectionType = automatic>
struct constructor_detection {
  private:
    using detection_type = std::conditional_t<
        has_constructor_typedef_v<T>, constructor_typedef<T>,
        default_constructor_detection<T, DetectionType>>;

  public:
    using arguments = constructor_detection_arguments_t<detection_type>;
    static constexpr constructor_kind kind = detection_type::kind;
    static constexpr size_t arity = detection_type::arity;

    template <typename Type, typename Context, typename Container>
    static auto construct(Context& ctx, Container& container) {
        return detection_type::template construct<Type>(ctx, container);
    }

    template <typename Type, typename Context, typename Container>
    static void construct(void* ptr, Context& ctx, Container& container) {
        detection_type::template construct<Type>(ptr, ctx, container);
    }
};

} // export namespace silicon::di

// --- factory/constructor.h ---



export namespace silicon::di {

template <typename...> struct constructor;

template <typename T> struct constructor<T> : constructor_detection<T> {};

template <typename T, typename... Args> struct constructor<T(Args...)> {
    using arguments = type_list<Args...>;
    static constexpr size_t arity = sizeof...(Args);
    static constexpr bool valid =
        is_list_initializable_v<T, Args...> ||
        is_direct_initializable_v<T, Args...>;

    template <typename Type, typename Context, typename Container>
    static auto construct(Context& ctx, Container& container) {
        return construction_dispatch<Type, T>::construct(
            ctx.template resolve<Args>(container)...);
    }

    template <typename Type, typename Context, typename Container>
    static void construct(void* ptr, Context& ctx, Container& container) {
        construction_dispatch<Type, T>::construct(
            ptr, ctx.template resolve<Args>(container)...);
    }
};

} // export namespace silicon::di


// ==============================================================================
// ==  storage  —  storage policies (shared / unique / external / cyclical)
// ==============================================================================

// --- storage/storage.h ---


export namespace silicon::di {

template <typename StorageTag, typename Type, typename U> struct conversions;
// Primary (forward) declaration carries default arguments so that the
// sentinel type `storage<void>` is a well-formed (incomplete) type-id. This is
// required for clang: MSVC accepts `storage<void>` from defaults supplied on a
// later definition, but clang requires the defaults on the first declaration.
// `storage<void>` is only ever used inside std::is_same_v sentinels, so an
// incomplete type is sufficient.
template <typename StorageTag, typename Type = void, typename StoredType = void,
          typename Factory = void, typename Conversions = void>
class storage;

template <typename StorageTag, typename Type, typename StoredType,
          typename Factory>
class storage_instance;

template <typename StorageTag, typename Type, typename TypeInterface>
struct storage_interface_requirements : std::bool_constant<true> {};

template <typename Storage, typename Type, typename TypeInterface>
inline constexpr bool storage_interface_requirements_v =
    storage_interface_requirements<Storage, Type, TypeInterface>::value;

} // export namespace silicon::di


// ==============================================================================
// ==  registration  —  type registration, annotations & requirements
// ==============================================================================

// --- registration/requirements.h ---




export namespace silicon::di {
template <typename... Args> struct interfaces;


template <typename Alternative, typename Interface, typename = void>
struct alternative_provides_interface
    : std::bool_constant<std::is_same_v<
          std::remove_cv_t<std::remove_reference_t<Alternative>>,
          std::remove_cv_t<std::remove_reference_t<Interface>>>> {};

template <typename Alternative, typename Interface>
struct alternative_provides_interface<
    Alternative, Interface,
    std::enable_if_t<type_traits<std::remove_cv_t<
                         std::remove_reference_t<Alternative>>>::enabled &&
                     type_traits<std::remove_cv_t<
                         std::remove_reference_t<Alternative>>>::is_value_borrowable>>
    : std::bool_constant<
          std::is_same_v<
              std::remove_cv_t<std::remove_reference_t<Alternative>>,
              std::remove_cv_t<std::remove_reference_t<Interface>>> ||
          alternative_provides_interface<
              typename type_traits<std::remove_cv_t<
                  std::remove_reference_t<Alternative>>>::value_type,
              Interface>::value> {};

template <typename Alternatives, typename Interface>
struct alternative_interface_match_count;

template <typename Interface, typename... Alternatives>
struct alternative_interface_match_count<type_list<Alternatives...>, Interface>
    : std::integral_constant<
          size_t,
          (0u + ... +
           (alternative_provides_interface<Alternatives, Interface>::value ? 1u
                                                                           : 0u))> {
};

template <typename Type, typename Interface, typename = void>
struct alternative_type_provides_interface : std::false_type {};

template <typename Type, typename Interface>
struct alternative_type_provides_interface<
    Type, Interface,
    std::enable_if_t<is_alternative_type_v<
        std::remove_cv_t<std::remove_reference_t<Type>>>>>
    : std::bool_constant<
          alternative_interface_match_count<
              alternative_type_alternatives_t<
                  std::remove_cv_t<std::remove_reference_t<Type>>>,
              Interface>::value == 1> {};

template <typename Storage, typename TypeInterface, typename Type>
struct interface_registration_requirements {
    using interface_type = typename annotated_traits<TypeInterface>::type;
    using normalized_type = normalized_type_t<Type>;
    using normalized_interface_type = normalized_type_t<interface_type>;
    using interface_value_type =
        std::remove_cv_t<std::remove_reference_t<interface_type>>;

    static constexpr bool is_alternative_type_interface =
        is_alternative_type_interface_compatible_v<normalized_type,
                                                   interface_value_type> ||
        is_alternative_type_interface_compatible_v<normalized_type,
                                                   normalized_interface_type> ||
        alternative_type_provides_interface<normalized_type,
                                            interface_value_type>::value ||
        alternative_type_provides_interface<normalized_type,
                                            normalized_interface_type>::value;
    static constexpr bool is_reference_interface =
        std::is_reference_v<interface_type>;
    static constexpr bool is_void_interface =
        std::is_void_v<normalized_interface_type>;
    static constexpr bool is_function_interface =
        std::is_function_v<interface_type>;
    static constexpr bool registered_type_is_complete =
        !requires_complete_type_v<normalized_type> ||
        is_complete_v<normalized_type>;
    static constexpr bool interface_type_is_complete =
        !requires_complete_type_v<normalized_interface_type> ||
        is_complete_v<normalized_interface_type>;

    static constexpr bool array_shape_matches = [] {
        if constexpr (!is_array_like_type_v<Type> ||
                      !std::is_array_v<interface_type>) {
            return true;
        } else {
            using exact_interface_type =
                array_like_exact_interface_type_t<Type>;
            return std::is_same_v<std::remove_cv_t<interface_type>,
                                  std::remove_cv_t<exact_interface_type>> ||
                   (std::is_array_v<Type> && (std::rank_v<Type> > 1) &&
                    std::is_same_v<std::remove_cv_t<interface_type>,
                                   std::remove_cv_t<std::remove_extent_t<Type>>>);
        }
    }();

    static constexpr bool array_element_matches = [] {
        if constexpr (!is_array_like_type_v<Type> ||
                      std::is_array_v<interface_type>) {
            return true;
        } else {
            return std::is_same_v<normalized_type, normalized_interface_type>;
        }
    }();

    static constexpr bool pointer_convertible =
        std::is_convertible_v<normalized_type*, normalized_interface_type*> ||
        is_alternative_type_interface;

    static constexpr bool storage_supported = [] {
        if constexpr (std::is_same_v<normalized_type,
                                     normalized_interface_type>) {
            return true;
        } else {
            return storage_interface_requirements_v<
                Storage, normalized_type, normalized_interface_type>;
        }
    }();

    static constexpr bool valid =
        !is_reference_interface && !is_void_interface &&
        !is_function_interface && registered_type_is_complete &&
        interface_type_is_complete && array_shape_matches &&
        array_element_matches && pointer_convertible && storage_supported;

    static constexpr void assert_valid() {
        static_assert(!is_reference_interface,
                      "interfaces must not contain reference types");
        static_assert(!is_void_interface,
                      "interfaces<void> is not a valid registration target");
        static_assert(!is_function_interface,
                      "interfaces must not contain function types");
        static_assert(registered_type_is_complete,
                      "registered types must be complete");
        static_assert(interface_type_is_complete,
                      "registered interfaces must be complete");
        if constexpr (is_array_like_type_v<Type>) {
            if constexpr (std::is_array_v<interface_type>) {
                static_assert(
                    array_shape_matches,
                    "array registrations require matching array-shape interfaces");
            } else {
                static_assert(
                    array_element_matches,
                    "array registrations require matching element-type interfaces");
            }
        }

        if constexpr (registered_type_is_complete) {
            if constexpr ((!is_array_like_type_v<Type>) ||
                          (std::is_array_v<interface_type> && array_shape_matches) ||
                          (!std::is_array_v<interface_type> && array_element_matches)) {
                static_assert(
                    pointer_convertible,
                    "registered type must be pointer-convertible to the interface");
                if constexpr (pointer_convertible &&
                              !std::is_same_v<normalized_type,
                                              normalized_interface_type>) {
                    static_assert(storage_supported, "storage requirements not met");
                }
            }
        }
    }
};

template <typename... Args>
inline constexpr bool has_explicit_void_interface_v =
    (std::is_same_v<std::decay_t<Args>, interfaces<void>> || ...);

template <typename Storage, typename TypeList, typename Type>
struct registration_requirements;

template <typename Storage, typename Type, typename... TypeInterfaces>
struct registration_requirements<Storage, type_list<TypeInterfaces...>, Type> {
    static constexpr bool valid =
        (interface_registration_requirements<Storage, TypeInterfaces,
                                             Type>::valid &&
         ...);

    static constexpr void assert_valid() {
        (interface_registration_requirements<Storage, TypeInterfaces,
                                             Type>::assert_valid(),
         ...);
    }
};

} // export namespace silicon::di


// ==============================================================================
// ==  factory  —  constructor detection, callable & function injection
// ==============================================================================

// --- factory/callable.h ---



export namespace silicon::di {


template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename Signature> struct callable_invoke;

template <typename R, typename... Args> struct callable_invoke<R(Args...)> {
    template <typename Fn, typename Context, typename Container>
    static decltype(auto) construct(Fn&& fn, Context& ctx,
                                    Container& container) {
        return std::invoke(std::forward<Fn>(fn),
                           ctx.template resolve<Args>(container)...);
    }

    template <typename Type, typename Fn, typename Context, typename Container>
    static void construct(void* ptr, Fn&& fn, Context& ctx,
                          Container& container) {
        new (ptr) normalized_type_t<Type>(
            construct(std::forward<Fn>(fn), ctx, container));
    }
};

template <typename R, typename... Args>
struct callable_invoke<R(Args...) noexcept> : callable_invoke<R(Args...)> {};

template <typename T, typename = void> struct callable_signature;

template <typename R, typename... Args>
struct callable_signature<R(Args...), void> {
    using type = R(Args...);
};

template <typename R, typename... Args>
struct callable_signature<R(Args...) noexcept, void> {
    using type = R(Args...) noexcept;
};

#define SILICON_DI_CALLABLE_SIGNATURE_VARIANTS(APPLY)                               \
    APPLY(, &, )                                                               \
    APPLY(, &, noexcept)                                                       \
    APPLY(, &&, )                                                              \
    APPLY(, &&, noexcept)                                                      \
    APPLY(const, , )                                                           \
    APPLY(const, , noexcept)                                                   \
    APPLY(const, &, )                                                          \
    APPLY(const, &, noexcept)                                                  \
    APPLY(const, &&, )                                                         \
    APPLY(const, &&, noexcept)                                                 \
    APPLY(volatile, , )                                                        \
    APPLY(volatile, , noexcept)                                                \
    APPLY(volatile, &, )                                                       \
    APPLY(volatile, &, noexcept)                                               \
    APPLY(volatile, &&, )                                                      \
    APPLY(volatile, &&, noexcept)                                              \
    APPLY(const volatile, , )                                                  \
    APPLY(const volatile, , noexcept)                                          \
    APPLY(const volatile, &, )                                                 \
    APPLY(const volatile, &, noexcept)                                         \
    APPLY(const volatile, &&, )                                                \
    APPLY(const volatile, &&, noexcept)

#define SILICON_DI_DEFINE_CALLABLE_SIGNATURE(cv_qualifier, ref_qualifier,           \
                                        noexcept_qualifier)                    \
    template <typename R, typename... Args>                                    \
    struct callable_signature<                                                 \
        R(Args...) cv_qualifier ref_qualifier noexcept_qualifier, void>        \
        : callable_signature<R(Args...) noexcept_qualifier> {};

SILICON_DI_CALLABLE_SIGNATURE_VARIANTS(SILICON_DI_DEFINE_CALLABLE_SIGNATURE)

#undef SILICON_DI_DEFINE_CALLABLE_SIGNATURE

template <typename T>
struct callable_signature<T*, std::enable_if_t<std::is_function_v<T>>>
    : callable_signature<T> {};

template <typename Signature>
struct callable_signature<std::function<Signature>, void>
    : callable_signature<Signature> {};

#if defined(__cpp_lib_move_only_function)
template <typename Signature>
struct callable_signature<std::move_only_function<Signature>, void>
    : callable_signature<Signature> {};
#endif

#if defined(__cpp_lib_copyable_function)
template <typename Signature>
struct callable_signature<std::copyable_function<Signature>, void>
    : callable_signature<Signature> {};
#endif

template <typename Class, typename R, typename... Args>
struct callable_signature<R (Class::*)(Args...), void>
    : callable_signature<R(Class&, Args...)> {};

template <typename Class, typename R, typename... Args>
struct callable_signature<R (Class::*)(Args...) noexcept, void>
    : callable_signature<R(Class&, Args...) noexcept> {};

template <typename T> struct callable_operator_signature;

template <typename Class, typename R, typename... Args>
struct callable_operator_signature<R (Class::*)(Args...)>
    : callable_signature<R(Args...)> {};

template <typename Class, typename R, typename... Args>
struct callable_operator_signature<R (Class::*)(Args...) noexcept>
    : callable_signature<R(Args...) noexcept> {};

#define SILICON_DI_MEMBER_CALLABLE_SIGNATURE_VARIANTS(APPLY)                        \
    APPLY(, &, Class&, )                                                       \
    APPLY(, &, Class&, noexcept)                                               \
    APPLY(, &&, Class&&, )                                                     \
    APPLY(, &&, Class&&, noexcept)                                             \
    APPLY(const, , const Class&, )                                             \
    APPLY(const, , const Class&, noexcept)                                     \
    APPLY(const, &, const Class&, )                                            \
    APPLY(const, &, const Class&, noexcept)                                    \
    APPLY(const, &&, const Class&&, )                                          \
    APPLY(const, &&, const Class&&, noexcept)                                  \
    APPLY(volatile, , volatile Class&, )                                       \
    APPLY(volatile, , volatile Class&, noexcept)                               \
    APPLY(volatile, &, volatile Class&, )                                      \
    APPLY(volatile, &, volatile Class&, noexcept)                              \
    APPLY(volatile, &&, volatile Class&&, )                                    \
    APPLY(volatile, &&, volatile Class&&, noexcept)                            \
    APPLY(const volatile, , const volatile Class&, )                           \
    APPLY(const volatile, , const volatile Class&, noexcept)                   \
    APPLY(const volatile, &, const volatile Class&, )                          \
    APPLY(const volatile, &, const volatile Class&, noexcept)                  \
    APPLY(const volatile, &&, const volatile Class&&, )                        \
    APPLY(const volatile, &&, const volatile Class&&, noexcept)

#define SILICON_DI_DEFINE_MEMBER_CALLABLE_SIGNATURE(cv_qualifier, ref_qualifier,    \
                                               object_type,                    \
                                               noexcept_qualifier)             \
    template <typename Class, typename R, typename... Args>                    \
    struct callable_signature<                                                 \
        R (Class::*)(Args...) cv_qualifier ref_qualifier noexcept_qualifier,   \
        void> : callable_signature<R(object_type, Args...) noexcept_qualifier> {};

SILICON_DI_MEMBER_CALLABLE_SIGNATURE_VARIANTS(SILICON_DI_DEFINE_MEMBER_CALLABLE_SIGNATURE)

#undef SILICON_DI_DEFINE_MEMBER_CALLABLE_SIGNATURE
#undef SILICON_DI_MEMBER_CALLABLE_SIGNATURE_VARIANTS

#define SILICON_DI_DEFINE_CALLABLE_OPERATOR_SIGNATURE(cv_qualifier, ref_qualifier,  \
                                                noexcept_qualifier)            \
    template <typename Class, typename R, typename... Args>                    \
    struct callable_operator_signature<                                        \
        R (Class::*)(Args...) cv_qualifier ref_qualifier noexcept_qualifier>   \
        : callable_signature<R(Args...) noexcept_qualifier> {};

SILICON_DI_CALLABLE_SIGNATURE_VARIANTS(SILICON_DI_DEFINE_CALLABLE_OPERATOR_SIGNATURE)

#undef SILICON_DI_DEFINE_CALLABLE_OPERATOR_SIGNATURE
#undef SILICON_DI_CALLABLE_SIGNATURE_VARIANTS

template <typename T>
struct callable_signature<T,
                          std::void_t<decltype(&remove_cvref_t<T>::operator())>>
    : callable_operator_signature<decltype(&remove_cvref_t<T>::operator())> {};

template <typename T>
using callable_signature_t =
    typename callable_signature<remove_cvref_t<T>>::type;

template <typename Signature, typename Callable>
struct callable_dispatch_signature {
    using type = Signature;
};

template <typename Callable>
struct callable_dispatch_signature<void, Callable> {
    using type = callable_signature_t<Callable>;
};

template <typename Signature, typename Callable>
using callable_dispatch_signature_t =
    typename callable_dispatch_signature<Signature, Callable>::type;

template <typename Signature, typename T> struct callable_factory {
    explicit callable_factory(T fn) : fn_(std::move(fn)) {}

    template <typename Type, typename Context, typename Container>
    auto construct(Context& ctx, Container& container) {
        return callable_invoke<Signature>::construct(fn_, ctx, container);
    }

    template <typename Type, typename Context, typename Container>
    void construct(void* ptr, Context& ctx, Container& container) {
        callable_invoke<Signature>::template construct<Type>(ptr, fn_, ctx,
                                                             container);
    }

  private:
    T fn_;
};


template <typename Signature = void, typename T> auto callable(T&& fn) {
    using fn_type = remove_cvref_t<T>;
    using dispatch_signature =
        callable_dispatch_signature_t<Signature, fn_type>;

    return callable_factory<dispatch_signature, fn_type>(
        std::forward<T>(fn));
}

} // export namespace silicon::di

// --- factory/function.h ---


export namespace silicon::di {

template <typename T, T fn> struct function_decl {
    template <typename Type, typename Context, typename Container>
    static auto construct(Context& ctx, Container& container) {
        return callable_invoke<
            callable_signature_t<T>>::construct(fn, ctx, container);
    }

    template <typename Type, typename Context, typename Container>
    static void construct(void* ptr, Context& ctx, Container& container) {
        callable_invoke<callable_signature_t<T>>::
            template construct<Type>(ptr, fn, ctx, container);
    }
};

template <auto fn> struct function : function_decl<decltype(fn), fn> {};

} // export namespace silicon::di


// ==============================================================================
// ==  core  —  binding model, binding collection & context
// ==============================================================================

// --- core/factory_traits.h ---




export namespace silicon::di {


template <typename, typename = void> struct has_factory_arguments : std::false_type {};

template <typename Factory>
struct has_factory_arguments<Factory, std::void_t<typename Factory::arguments>>
    : std::true_type {};

template <typename Factory> struct is_plain_constructor_factory : std::false_type {};

template <typename T>
struct is_plain_constructor_factory<constructor<T>>
    : std::bool_constant<!std::is_function_v<T>> {};

template <typename Signature> struct callable_dependencies;

template <typename R, typename... Args>
struct callable_dependencies<R(Args...)> {
    using type = type_list<Args...>;
};

template <typename R, typename... Args>
struct callable_dependencies<R(Args...) noexcept>
    : callable_dependencies<R(Args...)> {};

template <typename Signature>
using callable_dependencies_t =
    typename callable_dependencies<Signature>::type;

template <typename Factory, typename = void>
struct factory_arguments_or_void {
    using type = void;
};

template <typename Factory>
struct factory_arguments_or_void<Factory,
                                 std::void_t<typename Factory::arguments>> {
    using type = typename Factory::arguments;
};

template <typename Factory>
using factory_arguments_or_void_t =
    typename factory_arguments_or_void<Factory>::type;


template <typename Factory, typename = void> struct factory_traits {
    using dependencies = void;
    static constexpr bool has_explicit_dependencies = false;
    static constexpr bool is_compile_time_bindable = false;
};

template <typename Factory>
struct factory_traits<
    Factory,
    std::enable_if_t<has_factory_arguments<Factory>::value &&
                     !is_plain_constructor_factory<Factory>::value>> {
    using dependencies = typename Factory::arguments;
    static constexpr bool has_explicit_dependencies = true;
    static constexpr bool is_compile_time_bindable = true;
};

template <typename T>
struct factory_traits<constructor<T>, std::enable_if_t<!std::is_function_v<T>>> {
    using dependencies = factory_arguments_or_void_t<constructor<T>>;
    static constexpr bool has_explicit_dependencies = false;
    static constexpr bool is_compile_time_bindable =
        constructor<T>::kind == constructor_kind::kConcrete;
};

template <typename T, T fn>
struct factory_traits<function_decl<T, fn>> {
    using dependencies =
        callable_dependencies_t<callable_signature_t<T>>;
    static constexpr bool has_explicit_dependencies = true;
    static constexpr bool is_compile_time_bindable = true;
};

template <auto fn>
struct factory_traits<function<fn>>
    : factory_traits<function_decl<decltype(fn), fn>> {};

template <typename Signature, typename T>
struct factory_traits<callable_factory<Signature, T>> {
    using dependencies = callable_dependencies_t<Signature>;
    static constexpr bool has_explicit_dependencies = true;
    static constexpr bool is_compile_time_bindable = false;
};

} // export namespace silicon::di


// ==============================================================================
// ==  registration  —  type registration, annotations & requirements
// ==============================================================================

// --- registration/type_registration.h ---

// #include "silicon/di/core/config.h"


export namespace silicon::di {
struct unique;
template <typename... Registrations> struct static_registry;
template <typename T> struct storage_marker {
    using type = T;
    template <typename U> using rebind_t = storage_marker<U>;
};

template <typename T> struct scope {
    using type = T;
    template <typename U> using rebind_t = scope<U>;
};

template <typename... Args> struct interfaces {
    using type = type_list<Args...>;

    template <typename U> using rebind_t = interfaces<U>;
};

template <typename... Args> struct interfaces<type_list<Args...>> {
    using type = type_list<Args...>;

    template <typename U> using rebind_t = interfaces<U>;
};

template <typename T> struct factory {
    using type = T;
    template <typename U> using rebind_t = factory<U>;
};

template <typename T> struct key {
    using type = T;
    template <typename U> using rebind_t = key<U>;
};

template <typename T> struct conversions_marker {
    using type = T;
    template <typename U> using rebind_t = conversions_marker<U>;
};

template <typename... Args> struct dependencies {
    using type = type_list<Args...>;
    template <typename U> using rebind_t = dependencies<U>;
};

template <typename... Args> struct bindings {
    using type = static_registry<Args...>;
    template <typename U> using rebind_t = bindings<U>;
};

template <> struct bindings<void> {
    using type = void;
    template <typename U> using rebind_t = bindings<U>;
};

template <typename... Args> struct bindings<static_registry<Args...>> {
    using type = static_registry<Args...>;
    template <typename U> using rebind_t = bindings<U>;
};

template <typename T> struct static_bindings_source;

template <typename... Args>
struct static_bindings_source<bindings<Args...>> {
    using type = typename bindings<Args...>::type;
};

template <typename... Args>
struct static_bindings_source<static_registry<Args...>> {
    using type = static_registry<Args...>;
};

template <typename T>
using static_bindings_source_t = typename static_bindings_source<T>::type;

template <> struct dependencies<void> {
    using type = void;
    template <typename U> using rebind_t = dependencies<U>;
};

template <typename... Args> struct dependencies<type_list<Args...>> {
    using type = type_list<Args...>;
    template <typename U> using rebind_t = dependencies<U>;
};


template <typename Type, typename = void> struct deduced_interface_types {
    using type = type_list<std::remove_cv_t<std::remove_reference_t<Type>>>;
};

template <typename Type>
struct deduced_interface_types<
    Type,
    std::enable_if_t<type_traits<std::remove_cv_t<
                         std::remove_reference_t<Type>>>::enabled &&
                     type_traits<std::remove_cv_t<
                         std::remove_reference_t<Type>>>::is_value_borrowable>> {
  private:
    using value_type = std::remove_cv_t<std::remove_reference_t<Type>>;

  public:
    using type = type_list_cat_t<
        type_list<value_type>,
        typename deduced_interface_types<
            typename type_traits<value_type>::value_type>::type>;
};

template <typename Type>
struct deduced_interface_types<
    Type,
    std::enable_if_t<is_alternative_type_v<
        std::remove_cv_t<std::remove_reference_t<Type>>>>> {
  private:
    using value_type = std::remove_cv_t<std::remove_reference_t<Type>>;

    template <typename Alternatives> struct alternatives_interface_types;

    template <typename... Alternatives>
    struct alternatives_interface_types<type_list<Alternatives...>> {
        using type = type_list_cat_t<
            typename deduced_interface_types<Alternatives>::type...>;
    };

  public:
    using type = type_list_unique_t<type_list_cat_t<
        type_list<value_type>,
        typename alternatives_interface_types<
            alternative_type_alternatives_t<value_type>>::type>>;
};

template <typename Expected, typename Candidate, typename = void>
struct registration_arg_matches : std::false_type {};

template <typename Expected, typename Candidate>
struct registration_arg_matches<
    Expected, Candidate,
    std::void_t<typename Candidate::template rebind_t<void>>>
    : std::bool_constant<
          std::is_same_v<Expected, typename Candidate::template rebind_t<void>> &&
          !std::is_same_v<Candidate, Expected>> {};

template <typename Current, typename Expected, typename Candidate>
using registration_arg_t = std::conditional_t<
    !std::is_same_v<Current, Expected>, Current,
    std::conditional_t<registration_arg_matches<Expected, Candidate>::value,
                       Candidate, Current>>;

template <typename ScopeType, typename StorageType, typename FactoryType,
          typename InterfaceType, typename KeyType, typename ConversionsType,
          typename DependenciesType, typename BindingsType>
struct registration_args {
    using scope_type = ScopeType;
    using storage_type = StorageType;
    using factory_type = FactoryType;
    using interface_type = InterfaceType;
    using key_type = KeyType;
    using conversions_type = ConversionsType;
    using dependencies_type = DependenciesType;
    using bindings_type = BindingsType;
};

template <typename ParsedArgs, typename... Args> struct parse_registration_args;

template <typename ParsedArgs>
struct parse_registration_args<ParsedArgs> {
    using type = ParsedArgs;
};

template <typename ScopeType, typename StorageType, typename FactoryType,
          typename InterfaceType, typename KeyType, typename ConversionsType,
          typename DependenciesType, typename BindingsType, typename Head,
          typename... Tail>
struct parse_registration_args<
    registration_args<ScopeType, StorageType, FactoryType, InterfaceType,
                      KeyType, ConversionsType, DependenciesType, BindingsType>,
    Head, Tail...> {
  private:
    using parsed_head = registration_args<
        registration_arg_t<ScopeType, ::silicon::di::scope<void>, Head>,
        registration_arg_t<StorageType, ::silicon::di::storage_marker<void>, Head>,
        registration_arg_t<FactoryType, ::silicon::di::factory<void>, Head>,
        registration_arg_t<InterfaceType, ::silicon::di::interfaces<void>, Head>,
        registration_arg_t<KeyType, ::silicon::di::key<void>, Head>,
        registration_arg_t<ConversionsType, ::silicon::di::conversions_marker<void>, Head>,
        registration_arg_t<DependenciesType, ::silicon::di::dependencies<void>, Head>,
        registration_arg_t<BindingsType, ::silicon::di::bindings<void>, Head>>;

  public:
    using type = typename parse_registration_args<parsed_head, Tail...>::type;
};

template <typename... Args>
using parse_registration_args_t = typename parse_registration_args<
    registration_args<::silicon::di::scope<void>, ::silicon::di::storage_marker<void>,
                      ::silicon::di::factory<void>, ::silicon::di::interfaces<void>,
                      ::silicon::di::key<void>,
                      ::silicon::di::conversions_marker<void>,
                      ::silicon::di::dependencies<void>, ::silicon::di::bindings<void>>,
    Args...>::type;

template <typename T>
inline constexpr bool is_supported_registration_arg_v =
    registration_arg_matches<::silicon::di::scope<void>, T>::value ||
    registration_arg_matches<::silicon::di::storage_marker<void>, T>::value ||
    registration_arg_matches<::silicon::di::factory<void>, T>::value ||
    registration_arg_matches<::silicon::di::interfaces<void>, T>::value ||
    registration_arg_matches<::silicon::di::key<void>, T>::value ||
    registration_arg_matches<::silicon::di::conversions_marker<void>, T>::value ||
    registration_arg_matches<::silicon::di::dependencies<void>, T>::value ||
    registration_arg_matches<::silicon::di::bindings<void>, T>::value;

template <typename T, typename...> struct get_type;
template <typename T> struct get_type<T, type_list<>> {
    using type = T;
};

template <typename T, typename Head, typename... Tail>
struct get_type<T, type_list<Head, Tail...>> {
    using type = std::conditional_t<
        std::is_same_v<T, typename Head::template rebind_t<void>> &&
            !std::is_same_v<Head, T>, /* &&... is needed for interface deduction
                                       */
        Head, typename get_type<T, type_list<Tail...>>::type>;
};

template <typename T, typename... Args>
using get_type_t = typename get_type<T, Args...>::type;

template <typename ParsedArgs>
using registration_scope_t = typename ParsedArgs::scope_type;

template <typename ParsedArgs>
using registration_storage_t = std::conditional_t<
    !std::is_same_v<typename ParsedArgs::storage_type,
                    ::silicon::di::storage_marker<void>>,
    typename ParsedArgs::storage_type,
    ::silicon::di::storage_marker<typename ParsedArgs::factory_type::type>>;

template <typename ParsedArgs,
          typename Dependencies = typename ParsedArgs::dependencies_type>
struct registration_constructor_default {
    using type = ::silicon::di::constructor<
        leaf_type_t<typename registration_storage_t<ParsedArgs>::type>>;
};

template <typename ParsedArgs>
struct registration_constructor_default<ParsedArgs, ::silicon::di::dependencies<void>> {
    using type = ::silicon::di::constructor<
        leaf_type_t<typename registration_storage_t<ParsedArgs>::type>>;
};

template <typename ParsedArgs, typename... Args>
struct registration_constructor_default<ParsedArgs,
                                        ::silicon::di::dependencies<Args...>> {
    using type = ::silicon::di::constructor<
        leaf_type_t<typename registration_storage_t<ParsedArgs>::type>(Args...)>;
};

template <typename ParsedArgs>
using registration_factory_default_t =
    ::silicon::di::factory<typename registration_constructor_default<
        ParsedArgs, typename ParsedArgs::dependencies_type>::type>;

template <typename ParsedArgs>
using registration_factory_t = std::conditional_t<
    !std::is_same_v<typename ParsedArgs::factory_type,
                    ::silicon::di::factory<void>>,
    typename ParsedArgs::factory_type,
    registration_factory_default_t<ParsedArgs>>;

template <typename StorageType, typename ScopeType, typename = void>
struct deduced_interface_type {
    using type = ::silicon::di::interfaces<leaf_type_t<StorageType>>;
};

template <typename StorageType, typename ScopeType>
struct deduced_interface_type<
    StorageType, ScopeType,
    std::void_t<typename ::silicon::di::alternative_type_interface_types<
        std::remove_cv_t<std::remove_reference_t<StorageType>>>::type>> {
    using type = ::silicon::di::interfaces<
        typename ::silicon::di::deduced_interface_types<
            std::remove_cv_t<std::remove_reference_t<StorageType>>>::type>;
};

template <typename StorageType, typename ScopeType>
struct deduced_interface_type<
    StorageType, ScopeType,
    std::enable_if_t<!std::is_same_v<typename ScopeType::type, unique> &&
                     type_traits<std::remove_cv_t<
                         std::remove_reference_t<StorageType>>>::enabled &&
                     type_traits<std::remove_cv_t<
                         std::remove_reference_t<StorageType>>>::is_value_borrowable &&
                     is_alternative_type_v<std::remove_cv_t<leaf_type_t<StorageType>>>>> {
    using type = ::silicon::di::interfaces<
        typename ::silicon::di::deduced_interface_types<
            std::remove_cv_t<leaf_type_t<StorageType>>>::type>;
};

template <typename StorageType, typename ScopeType>
struct deduced_interface_type<
    StorageType, ScopeType,
    std::enable_if_t<!std::is_same_v<typename ScopeType::type, unique> &&
                     type_traits<StorageType>::enabled &&
                     !std::is_pointer_v<StorageType> &&
                     std::is_array_v<typename type_traits<StorageType>::value_type>>> {
    using type = ::silicon::di::interfaces<typename type_traits<StorageType>::value_type,
                                     leaf_type_t<StorageType>>;
};

template <typename StorageType, typename ScopeType>
struct deduced_interface_type<
    StorageType, ScopeType,
    std::enable_if_t<std::is_array_v<StorageType> &&
                     (std::rank_v<StorageType> > 1)>> {
    using type = ::silicon::di::interfaces<std::remove_extent_t<StorageType>,
                                     StorageType, leaf_type_t<StorageType>>;
};

template <typename StorageType, typename ScopeType>
struct deduced_interface_type<
    StorageType, ScopeType,
    std::enable_if_t<std::is_array_v<StorageType> &&
                     (std::rank_v<StorageType> == 1)>> {
    using type = ::silicon::di::interfaces<StorageType, leaf_type_t<StorageType>>;
};

template <typename ParsedArgs>
using registration_interface_from_storage_t = typename deduced_interface_type<
    typename registration_storage_t<ParsedArgs>::type,
    registration_scope_t<ParsedArgs>>::type;

template <typename ParsedArgs>
using registration_interface_from_factory_t = ::silicon::di::interfaces<leaf_type_t<
    typename registration_factory_t<ParsedArgs>::type>>;

template <typename ParsedArgs>
using registration_interface_t = std::conditional_t<
    !std::is_same_v<typename ParsedArgs::interface_type,
                    ::silicon::di::interfaces<void>>,
    typename ParsedArgs::interface_type,
    std::conditional_t<
        !std::is_same_v<registration_interface_from_storage_t<ParsedArgs>,
                        ::silicon::di::interfaces<void>>,
        registration_interface_from_storage_t<ParsedArgs>,
        registration_interface_from_factory_t<ParsedArgs>>>;

template <typename ParsedArgs>
using registration_conversions_t = std::conditional_t<
    !std::is_same_v<typename ParsedArgs::conversions_type,
                    ::silicon::di::conversions_marker<void>>,
    typename ParsedArgs::conversions_type,
    ::silicon::di::conversions_marker<conversions<
        typename registration_scope_t<ParsedArgs>::type,
        typename registration_storage_t<ParsedArgs>::type, runtime_type>>>;

template <typename T> struct normalize_dependencies {
    using type = ::silicon::di::dependencies<T>;
};

template <typename... Args>
struct normalize_dependencies<type_list<Args...>> {
    using type = ::silicon::di::dependencies<Args...>;
};

template <typename T>
using normalize_dependencies_t = typename normalize_dependencies<T>::type;

template <typename ParsedArgs>
using registration_dependencies_t = std::conditional_t<
    !std::is_same_v<typename ParsedArgs::dependencies_type,
                    ::silicon::di::dependencies<void>>,
    typename ParsedArgs::dependencies_type,
    std::conditional_t<
        !std::is_same_v<typename ::silicon::di::factory_traits<
                            typename registration_factory_t<ParsedArgs>::type>::dependencies,
                        void>,
        normalize_dependencies_t<typename ::silicon::di::factory_traits<
            typename registration_factory_t<ParsedArgs>::type>::dependencies>,
        ::silicon::di::dependencies<void>>>;

template <typename ParsedArgs>
using registration_key_t = std::conditional_t<
    !std::is_same_v<typename ParsedArgs::key_type, ::silicon::di::key<void>>,
    typename ParsedArgs::key_type,
    ::silicon::di::key<void>>;



// TODO:
// the default factory is hardcoded

template <typename... Args> struct type_registration {
  private:
    // Parse the explicit registration wrappers once, then derive any defaults
    // from that cached result instead of rescanning Args... for each role.
    using parsed_args = parse_registration_args_t<Args...>;

  public:
    // Scope has to be scpecified as there is no way how to deduce it
    using scope_type = registration_scope_t<parsed_args>;
    static_assert((is_supported_registration_arg_v<Args> && ...),
                  "type_registration expects scope/storage/factory/interfaces/key/conversions/dependencies/bindings wrappers");
    static_assert(!std::is_same_v<scope_type, scope<void>>,
                  "failed to deduce a scope type");

    // Storage can be deduced from Factory
    using storage_type = registration_storage_t<parsed_args>;
    static_assert(!std::is_same_v<storage_type, storage<void>>,
                  "failed to deduce a storage type");

    // Factory can be deduced from Storage
    using factory_type = registration_factory_t<parsed_args>;
    static_assert(!std::is_same_v<factory_type, factory<void>>,
                  "failed to deduce a factory type");

    // Interface can be deduced from Storage or Factory
    using interface_type = registration_interface_t<parsed_args>;
    static_assert(!std::is_same_v<interface_type, interfaces<void>>,
                  "failed to deduce an interface type");

    using key_type = registration_key_t<parsed_args>;

    // Conversions are deduced from Storage and Scope
    using conversions_type = registration_conversions_t<parsed_args>;
    static_assert(!std::is_same_v<conversions_type, conversions_marker<void>>,
                  "failed to deduce a conversions type");

    using dependencies_type = registration_dependencies_t<parsed_args>;
    using bindings_type = typename parsed_args::bindings_type;
};

} // export namespace silicon::di


// ==============================================================================
// ==  core  —  binding model, binding collection & context
// ==============================================================================

// --- core/binding_model.h ---




export namespace silicon::di {


template <typename Interface, typename BindingModel> struct binding {
    using interface_type = Interface;
    using binding_model_type = BindingModel;
    using storage_type = typename BindingModel::storage_type;
    using key_type = typename BindingModel::key_type;
};

template <typename Registration, bool StorageTagIsComplete>
struct binding_model_impl {
    using registration_type = Registration;
    using interface_types = typename Registration::interface_type::type;
    using registered_storage_type = typename Registration::storage_type::type;
    using scope_type = typename Registration::scope_type;
    using storage_tag = typename scope_type::type;
    using factory_type = typename Registration::factory_type::type;
    using key_type = typename Registration::key_type::type;
    using conversions_type = typename Registration::conversions_type::type;
    using dependencies_type = typename Registration::dependencies_type;
    using bindings_type = typename Registration::bindings_type::type;

    static constexpr bool storage_tag_is_complete = StorageTagIsComplete;
    static constexpr bool use_interface_as_stored_leaf = false;
    using stored_leaf_type = leaf_type_t<registered_storage_type>;
    using stored_type = rebind_leaf_t<registered_storage_type, stored_leaf_type>;
    using storage_type = void;
    using requirements = void;
    static constexpr bool valid = false;
};

template <typename Registration>
struct binding_model_impl<Registration, true> {
    using registration_type = Registration;
    using interface_types = typename Registration::interface_type::type;
    using registered_storage_type = typename Registration::storage_type::type;
    using scope_type = typename Registration::scope_type;
    using storage_tag = typename scope_type::type;
    using factory_type = typename Registration::factory_type::type;
    using key_type = typename Registration::key_type::type;
    using conversions_type = typename Registration::conversions_type::type;
    using dependencies_type = typename Registration::dependencies_type;
    using bindings_type = typename Registration::bindings_type::type;

    static constexpr bool storage_tag_is_complete = true;
    static constexpr bool use_interface_as_stored_leaf =
        use_interface_as_stored_leaf_v<registered_storage_type, interface_types>;

    using stored_leaf_type =
        std::conditional_t<use_interface_as_stored_leaf,
                           typename annotated_traits<
                               type_list_head_t<interface_types>>::type,
                           leaf_type_t<registered_storage_type>>;
    using stored_type =
        rebind_leaf_t<registered_storage_type, stored_leaf_type>;
    using storage_type =
        storage<storage_tag, registered_storage_type, stored_type,
                factory_type, conversions_type>;
    using requirements =
        registration_requirements<storage_type, interface_types,
                                  typename storage_type::type>;

    static constexpr bool valid = requirements::valid;
};

template <typename Registration>
using binding_model = binding_model_impl<
    Registration,
    !requires_complete_type_v<typename Registration::scope_type::type> ||
        is_complete_v<typename Registration::scope_type::type>>;

template <typename BindingModel, typename InterfaceList>
struct binding_expansion_impl;

template <typename BindingModel, typename... Interfaces>
struct binding_expansion_impl<BindingModel, type_list<Interfaces...>> {
    using interface_bindings =
        type_list<binding<Interfaces, BindingModel>...>;
};

template <typename BindingModel>
using binding_expansion =
    binding_expansion_impl<BindingModel, typename BindingModel::interface_types>;


} // export namespace silicon::di

// --- core/binding_selection.h ---



export namespace silicon::di {


enum class binding_selection_status {
    kFound,
    kNotFound,
    kAmbiguous,
};

template <binding_selection_status Status, typename Binding = void>
struct binding_choice {
    static constexpr binding_selection_status status = Status;
    static constexpr bool found = Status == binding_selection_status::kFound;
    using binding_type = Binding;
};

template <typename Binding>
using found_binding_choice_t =
    binding_choice<binding_selection_status::kFound, Binding>;

using missing_binding_choice_t =
    binding_choice<binding_selection_status::kNotFound>;

using ambiguous_binding_choice_t =
    binding_choice<binding_selection_status::kAmbiguous>;

template <typename Bindings> struct static_binding;

template <> struct static_binding<type_list<>> {
    using type = missing_binding_choice_t;
};

template <typename Binding> struct static_binding<type_list<Binding>> {
    using type = found_binding_choice_t<Binding>;
};

template <typename Binding0, typename Binding1, typename... Bindings>
struct static_binding<type_list<Binding0, Binding1, Bindings...>> {
    using type = ambiguous_binding_choice_t;
};

template <typename Bindings>
using static_binding_t = typename static_binding<Bindings>::type;

template <typename Binding, typename State = std::nullptr_t>
struct runtime_binding_selection {
    binding_selection_status status = binding_selection_status::kNotFound;
    Binding* binding = nullptr;
    State state = nullptr;

    constexpr bool found() const {
        return status == binding_selection_status::kFound;
    }

    constexpr bool ambiguous() const {
        return status == binding_selection_status::kAmbiguous;
    }

    static constexpr runtime_binding_selection found(Binding& binding,
                                                     State state = nullptr) {
        return {binding_selection_status::kFound, &binding, state};
    }

    static constexpr runtime_binding_selection miss() { return {}; }

    static constexpr runtime_binding_selection ambiguity() {
        return {binding_selection_status::kAmbiguous, nullptr, nullptr};
    }
};

template <typename Binding, typename State = std::nullptr_t>
constexpr runtime_binding_selection<Binding, State>
make_runtime_selection(Binding* binding, State state = nullptr) {
    return binding ? runtime_binding_selection<Binding, State>::found(*binding,
                                                                      state)
                   : runtime_binding_selection<Binding, State>::miss();
}

template <typename Binding, typename State = std::nullptr_t, typename Visitor>
constexpr runtime_binding_selection<Binding, State>
make_runtime_selection(Visitor&& visit_candidates) {
    Binding* selected_binding = nullptr;
    State selected_state = nullptr;
    std::size_t matches = 0;

    std::forward<Visitor>(visit_candidates)(
        [&](Binding& binding, State state = nullptr) {
            ++matches;
            if (matches == 1) {
                selected_binding = &binding;
                selected_state = state;
            }
        });

    if (matches == 0) {
        return runtime_binding_selection<Binding, State>::miss();
    }

    if (matches == 1) {
        return runtime_binding_selection<Binding, State>::found(
            *selected_binding, selected_state);
    }

    return runtime_binding_selection<Binding, State>::ambiguity();
}


} // export namespace silicon::di

// --- core/binding_resolution_policy.h ---


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

export namespace silicon::di {

enum class binding_resolution_policy {
    kPreferPrimary,
    kAmbiguousOnConflict,
};

enum class binding_result {
    kPrimary,
    kSecondary,
    kMissing,
    kAmbiguous,
};

struct binding_source_selection {
    binding_result result;

    constexpr bool found() const {
        return result == binding_result::kPrimary ||
               result == binding_result::kSecondary;
    }

    constexpr bool ambiguous() const {
        return result == binding_result::kAmbiguous;
    }

    constexpr bool primary() const {
        return result == binding_result::kPrimary;
    }

    constexpr bool secondary() const {
        return result == binding_result::kSecondary;
    }
};

constexpr binding_result resolve_binding(binding_selection_status primary,
                                         binding_selection_status secondary,
                                         binding_resolution_policy policy) {
    const bool primary_ambiguous =
        primary == binding_selection_status::kAmbiguous;
    const bool secondary_ambiguous =
        secondary == binding_selection_status::kAmbiguous;
    const bool primary_found = primary == binding_selection_status::kFound;
    const bool secondary_found = secondary == binding_selection_status::kFound;

    if (policy == binding_resolution_policy::kPreferPrimary) {
        if (primary_ambiguous) {
            return binding_result::kAmbiguous;
        }

        if (primary_found) {
            return binding_result::kPrimary;
        }

        if (secondary_ambiguous) {
            return binding_result::kAmbiguous;
        }

        if (secondary_found) {
            return binding_result::kSecondary;
        }

        return binding_result::kMissing;
    }

    if (primary_ambiguous || secondary_ambiguous ||
        (primary_found && secondary_found)) {
        return binding_result::kAmbiguous;
    }

    if (primary_found) {
        return binding_result::kPrimary;
    }

    if (secondary_found) {
        return binding_result::kSecondary;
    }

    return binding_result::kMissing;
}

constexpr binding_selection_status binding_status(binding_result resolution) {
    switch (resolution) {
    case binding_result::kPrimary:
    case binding_result::kSecondary:
        return binding_selection_status::kFound;
    case binding_result::kAmbiguous:
        return binding_selection_status::kAmbiguous;
    case binding_result::kMissing:
    default:
        return binding_selection_status::kNotFound;
    }
}

template <binding_selection_status SecondaryStatus>
constexpr binding_selection_status
resolve_binding_status(binding_selection_status primary,
                       binding_resolution_policy policy) {
    return binding_status(resolve_binding(primary, SecondaryStatus, policy));
}

template <typename ErrorRequest, typename ResolveRequest = ErrorRequest,
          typename Context, typename Sources>
as_expected_t<ResolveRequest> resolve_from_binding_sources(Context& context,
                                            Sources& sources) {
    auto selection = sources.select();
    if (selection.ambiguous()) {
        return std::unexpected(
            make_type_ambiguous_exception<ErrorRequest>(context));
    }

    if (selection.found()) {
        return sources.template resolve_selected<ResolveRequest>(context,
                                                                 selection);
    }

    return sources.template resolve_missing<ResolveRequest>(context);
}

template <typename PrimarySource, typename SecondarySource,
          typename MissingSource>
struct two_binding_sources {
    PrimarySource& primary;
    SecondarySource& secondary;
    MissingSource& missing;
    binding_resolution_policy policy;

    binding_source_selection select() {
        return {resolve_binding(primary.status(), secondary.status(), policy)};
    }

    template <typename Request, typename Context>
    decltype(auto) resolve_selected(Context& context,
                                    binding_source_selection selection) {
        if constexpr (PrimarySource::can_resolve) {
            if (selection.primary()) {
                return primary.template resolve<Request>(context);
            }
        }

        if constexpr (SecondarySource::can_resolve) {
            if (selection.secondary()) {
                return secondary.template resolve<Request>(context);
            }
        }

        return missing.template resolve<Request>(context);
    }

    template <typename Request, typename Context>
    decltype(auto) resolve_missing(Context& context) {
        return missing.template resolve<Request>(context);
    }
};

template <typename PrimarySource, typename SecondarySource,
          typename MissingSource>
two_binding_sources<PrimarySource, SecondarySource, MissingSource>
make_two_binding_sources(PrimarySource& primary, SecondarySource& secondary,
                         MissingSource& missing,
                         binding_resolution_policy policy) {
    return {primary, secondary, missing, policy};
}

template <typename Source, typename MissingSource> struct one_binding_source {
    Source& source;
    MissingSource& missing;

    binding_source_selection select() {
        const auto status = source.status();
        if (status == binding_selection_status::kAmbiguous) {
            return {binding_result::kAmbiguous};
        }
        if (status == binding_selection_status::kFound) {
            return {binding_result::kPrimary};
        }
        return {binding_result::kMissing};
    }

    template <typename Request, typename Context>
    decltype(auto) resolve_selected(Context& context,
                                    binding_source_selection selection) {
        if constexpr (Source::can_resolve) {
            if (selection.found()) {
                return source.template resolve<Request>(context);
            }
        }

        return missing.template resolve<Request>(context);
    }

    template <typename Request, typename Context>
    decltype(auto) resolve_missing(Context& context) {
        return missing.template resolve<Request>(context);
    }
};

template <typename Source, typename MissingSource>
one_binding_source<Source, MissingSource>
make_one_binding_source(Source& source, MissingSource& missing) {
    return {source, missing};
}

template <typename SelectedSource, typename MissingSource>
struct selected_binding_sources {
    SelectedSource& selected;
    MissingSource& missing;

    decltype(auto) select() { return selected.select(); }

    template <typename Request, typename Context, typename Selection>
    decltype(auto) resolve_selected(Context& context, Selection selection) {
        return selected.template resolve<Request>(context, selection);
    }

    template <typename Request, typename Context>
    decltype(auto) resolve_missing(Context& context) {
        return missing.template resolve<Request>(context);
    }
};

template <typename SelectedSource, typename MissingSource>
selected_binding_sources<SelectedSource, MissingSource>
make_selected_binding_sources(SelectedSource& selected,
                              MissingSource& missing) {
    return {selected, missing};
}

template <typename LookupRequest> struct missing_binding_source {
    template <typename ResolveRequest, typename Context>
    as_expected_t<ResolveRequest> resolve(Context& context) {
        (void)context;
        return std::unexpected(
            make_type_not_found_exception<LookupRequest>());
    }

    template <typename ResolveRequest, typename Context>
    as_expected_t<ResolveRequest> resolve_missing(Context& context) {
        (void)context;
        return std::unexpected(
            make_type_not_found_exception<LookupRequest>());
    }
};

} // export namespace silicon::di

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// --- core/none.h ---


export namespace silicon::di {

struct none_t {};

template <typename T> struct is_none : std::bool_constant<false> {};
template <> struct is_none<none_t> : std::bool_constant<true> {};
template <typename T> inline constexpr auto is_none_v = is_none<T>::value;

} // export namespace silicon::di


// ==============================================================================
// ==  factory  —  constructor detection, callable & function injection
// ==============================================================================

// --- factory/invoke.h ---



export namespace silicon::di {

template <typename T> struct factory_invoke {
    template <typename Context, typename Container, typename Callable>
    static decltype(auto) construct(Context& ctx, Container& container,
                                    Callable&& callable) {
        return callable_invoke<callable_signature_t<T>>::
            construct(std::forward<Callable>(callable), ctx, container);
    }
};

} // export namespace silicon::di


// ==============================================================================
// ==  memory  —  allocators, arena & object lifetime
// ==============================================================================

// --- memory/allocator.h ---



export namespace silicon::di {
template <typename Allocator> struct allocator_base : public Allocator {
    template <typename AllocatorT>
    allocator_base(AllocatorT&& alloc)
        : Allocator(std::forward<Allocator>(alloc)) {}

    Allocator& get_allocator() { return *this; }
};

struct allocator_traits {
    template <typename T, typename Allocator>
    static auto rebind(Allocator& alloc) {
        return
            typename std::allocator_traits<Allocator>::template rebind_alloc<T>(
                alloc);
    }

    template <typename Allocator>
    static auto allocate(Allocator& alloc, size_t n) {
        return std::allocator_traits<Allocator>::allocate(alloc, n);
    }

    template <typename Allocator, typename T, typename... Args>
    static void construct(Allocator& alloc, T* ptr, Args&&... args) {
        return std::allocator_traits<Allocator>::construct(
            alloc, ptr, std::forward<Args>(args)...);
    }

    template <typename Allocator, typename T>
    static void destroy(Allocator& alloc, T* ptr) {
        std::allocator_traits<Allocator>::destroy(alloc, ptr);
    }

    template <typename Allocator, typename T>
    static void deallocate(Allocator& alloc, T* ptr, size_t n) {
        std::allocator_traits<Allocator>::deallocate(alloc, ptr, n);
    }
};
} // export namespace silicon::di


// ==============================================================================
// ==  index  —  index collections (array / map / unordered_map)
// ==============================================================================

// --- index/index.h ---




export namespace silicon::di {
template <typename Arg, typename Definitions> struct index_tag;
template <typename Definitions, typename Value, typename Allocator>
struct index;

template <typename Key, typename Value, typename Allocator, typename Tag>
struct index_collection;


template <typename Entry> struct index_entry;

template <typename Key, typename Tag> struct index_entry<type_list<Key, Tag>> {
    using key = Key;
    using tag = Tag;
};

template <typename Arg, typename Definitions> struct index_tag_impl;

template <typename Arg, typename Head, typename... Tail>
struct index_tag_impl<Arg, type_list<Head, Tail...>> {
    using type =
        std::conditional_t<std::is_same_v<typename index_entry<Head>::key, Arg>,
                           typename index_entry<Head>::tag,
                           typename index_tag_impl<Arg, type_list<Tail...>>::type>;
};

template <typename Arg> struct index_tag_impl<Arg, type_list<>> {
    using type = void;
};

template <typename Definitions, typename Value, typename Allocator>
struct index_impl;

template <typename Value, typename Allocator>
struct index_impl<type_list<>, Value, Allocator> {
    index_impl(Allocator&) {}
};

template <typename Value, typename Allocator, typename... Entries>
struct index_impl<type_list<Entries...>, Value, Allocator> {
    index_impl(Allocator&) {}

    template <typename T> struct index_ptr : allocator_base<Allocator> {
        // TODO: this pattern already exists in the constructor-backed
        // instance holder; try to merge them.
        index_ptr(Allocator& al) : allocator_base<Allocator>(al) {
            auto alloc = allocator_traits::rebind<T>(this->get_allocator());
            try {
                index_ = allocator_traits::allocate(alloc, 1);
                allocator_traits::construct(alloc, index_,
                                            this->get_allocator());
            } catch (...) {
                allocator_traits::deallocate(alloc, index_, 1);
                throw;
            }
        }

        ~index_ptr() {
            if (index_) {
                auto alloc = allocator_traits::rebind<T>(this->get_allocator());
                allocator_traits::destroy(alloc, index_);
                allocator_traits::deallocate(alloc, index_, 1);
            }
        }

        index_ptr() = default;
        index_ptr(const index_ptr<T>&) = delete;
        index_ptr(index_ptr<T>&& other)
            : allocator_base<Allocator>(std::move(other)) {
            std::swap(index_, other.index_);
        }

        T& operator*() {
            assert(index_);
            return *index_;
        }

      private:
        T* index_ = nullptr;
    };

    template <typename Key> auto& get_index(Allocator& allocator) {
        using index_type = index_collection<
            Key, Value, Allocator,
            typename index_tag_impl<Key, type_list<Entries...>>::type>;

        if (indexes_.index() == 0) {
            indexes_.template emplace<index_ptr<index_type>>(allocator);
        }

        return *std::get<index_ptr<index_type>>(indexes_);
    }

  private:
    std::variant<std::monostate,
                 index_ptr<index_collection<typename index_entry<Entries>::key,
                                            Value, Allocator,
                                            typename index_entry<Entries>::tag>>...>
        indexes_;
};


template <typename Arg, typename... Entries>
struct index_tag<Arg, std::tuple<Entries...>>
    : index_tag_impl<Arg, type_list<to_type_list_t<Entries>...>> {};

template <typename Value, typename Allocator, typename... Entries>
struct index<std::tuple<Entries...>, Value, Allocator>
    : index_impl<type_list<to_type_list_t<Entries>...>, Value,
                         Allocator> {
    using index_impl<type_list<to_type_list_t<Entries>...>, Value,
                             Allocator>::index_impl;
};

} // export namespace silicon::di


// ==============================================================================
// ==  memory  —  allocators, arena & object lifetime
// ==============================================================================

// --- memory/aligned_storage.h ---


export namespace silicon::di {
template <std::size_t Len, std::size_t Alignment> struct aligned_storage {
    struct type {
        // TODO: force the storage type to be initialized to avoid warning
        // https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3074r2.html
        type() {}
    private:
        alignas(Alignment) std::uint8_t data[Len];
    };
};

template <std::size_t Len, std::size_t Alignment>
using aligned_storage_t = typename aligned_storage<Len, Alignment>::type;

template <std::size_t MinLen, typename... Ts> struct aligned_union {
    static constexpr std::size_t length = std::max({MinLen, sizeof(Ts)...});
    static constexpr std::size_t alignment = std::max({alignof(Ts)...});

    using type = typename aligned_storage<length, alignment>::type;
};

} // export namespace silicon::di

// --- memory/static_allocator.h ---



export namespace silicon::di {

template <typename T, typename Tag> class static_allocator {
  public:
    using value_type = T;

    static_allocator() noexcept {}
    template <typename U>
    static_allocator(const static_allocator<U, Tag>&) noexcept {}

    value_type* allocate(std::size_t n) {
        (void)n;
        assert(n == 1);
        if (used_)
            return nullptr;
        used_ = true;
        return reinterpret_cast<value_type*>(&storage_);
    }

    void deallocate(value_type* p, std::size_t n) noexcept {
        (void)p;
        (void)n;
        assert(used_);
        assert(n == 1);
        assert(p == reinterpret_cast<value_type*>(&storage_));
        used_ = false;
    }

  private:
    static silicon::di::aligned_storage_t<sizeof(T), alignof(T)> storage_;
    static bool used_;
};

template <typename T, typename Tag> bool static_allocator<T, Tag>::used_;
template <typename T, typename Tag>
silicon::di::aligned_storage_t<sizeof(T), alignof(T)>
    static_allocator<T, Tag>::storage_;

template <typename Allocator>
struct is_static_allocator : std::bool_constant<false> {};

template <typename T, typename Tag>
struct is_static_allocator<static_allocator<T, Tag>>
    : std::bool_constant<true> {};

template <typename Allocator>
inline constexpr bool is_static_allocator_v =
    is_static_allocator<Allocator>::value;

} // export namespace silicon::di

// --- memory/arena_allocator.h ---


export namespace silicon::di {

template< typename T > struct arena_allocator_traits: std::allocator_traits< T > {
    static constexpr intptr_t page_size() { return 1<<12; }
    static constexpr intptr_t header_size() { return sizeof(uintptr_t) * 2; }
};

template< typename Allocator = std::allocator<uint8_t> > class arena
    : arena_allocator_traits< Allocator >::template rebind_alloc<uint8_t>
{
    using allocator_type = typename arena_allocator_traits< Allocator >::template rebind_alloc<uint8_t>;
    using allocator_traits_type = arena_allocator_traits< allocator_type >;

    static constexpr std::size_t MaxBlockSize = 1<<21;

    // TODO: all the state here is quite big

    struct block {
        block* next;
        uintptr_t size:63;
        uintptr_t owned:1;
    };

    block* block_initial_ = nullptr;
    std::size_t block_size_ = 0;

    struct state {
        block* block_head_ = nullptr;
        intptr_t block_ptr_ = 0;
        intptr_t block_end_ = 0;
        intptr_t block_size_ = 0;
    };

    state state_;

    bool request_block(intptr_t bytes) {
        assert(state_.block_size_ > 0);

        // For large blocks, glibc's malloc is aligning large allocations
        // to the multiples of page size, also keeping space for chunk size.
        auto header_size = allocator_traits_type::header_size();
        auto page_size = allocator_traits_type::page_size();
        intptr_t size = ((
            header_size +
            std::max<intptr_t>(state_.block_size_, sizeof(block) + bytes) +
            page_size - 1
        ) & ~(page_size - 1)) - header_size;
        if (size < 0)
            return false;
        assert(size - (intptr_t)sizeof(block) >= bytes);
        auto head = allocate_block(size);
        head->size = size;
        head->owned = true;
        push_block(head);
        state_.block_size_ = std::min<intptr_t>(state_.block_size_ * 2, MaxBlockSize);
        return true;
    }

    block* allocate_block(intptr_t size) {
        block *ptr = reinterpret_cast<block*>(allocator_traits_type::allocate(*this, size));
        assert((reinterpret_cast<intptr_t>(ptr) & (alignof(block) - 1)) == 0);
        return ptr;
    }

    void push_block(block* head) {
        head->next = state_.block_head_;
        state_.block_head_ = head;
        state_.block_ptr_ = reinterpret_cast<intptr_t>(head) + sizeof(block);
        state_.block_end_ = reinterpret_cast<intptr_t>(head) + head->size;
    }

    void deallocate_block(block* ptr) {
        assert(ptr->owned);
        allocator_traits_type::deallocate(*this, reinterpret_cast<uint8_t*>(ptr), ptr->size);
    }

    void deallocate_blocks(block* end) {
        auto head = state_.block_head_;
        while(head != end) {
            assert(head->owned || !head->next);
            auto next = head->next;
            if (head->owned) {
                deallocate_block(head);
            } else {
                assert(head->next == nullptr);
                break;
            }
            head = next;
        }
    }

public:
    arena(std::size_t block_size)
        : block_size_(block_size)
    {
        state_.block_size_ = block_size;
    }

    template< typename T, std::size_t N > arena(T(&buffer)[N], std::size_t block_size = N * sizeof(T))
        : arena(reinterpret_cast<uint8_t*>(buffer), N * sizeof(T), block_size) {
        static_assert(std::is_trivially_default_constructible_v<T> &&
                      std::is_trivially_copyable_v<T>);
    }

    template< typename T > arena(T& buffer, std::size_t block_size = sizeof(T))
        : arena(reinterpret_cast<uint8_t*>(&buffer), sizeof(T), block_size) {
        static_assert(std::is_trivially_destructible_v<T>);
    }

    arena(void* buffer, std::size_t size)
        : arena(buffer, size, size)
    {}

    arena(void* buffer, std::size_t size, std::size_t block_size)
        : arena(block_size)
    {
        assert(size > sizeof(block));
        auto head = reinterpret_cast<block*>(buffer);
        block_initial_ = head;
        head->owned = false;
        head->size = size;
        push_block(head);
    }

    ~arena() {
        deallocate_blocks(nullptr);
    }

    void* allocate(intptr_t size, intptr_t alignment) {
        assert((alignment & (alignment - 1)) == 0);
        intptr_t padding = -state_.block_ptr_ & (alignment - 1);
        intptr_t capacity = state_.block_end_ - state_.block_ptr_ - padding;
        if (capacity < size) {
            if (std::numeric_limits<intptr_t>::max() - (alignment - 1) < size)
                return nullptr;
            if (!request_block(size + (alignment - 1)))
                return nullptr;
            padding = -state_.block_ptr_ & (alignment - 1);
            assert(size <= state_.block_end_ - state_.block_ptr_ - padding);
        }
        intptr_t ptr = state_.block_ptr_ + padding;
        assert((ptr & (alignment - 1)) == 0);
        state_.block_ptr_ = ptr + size;
        return reinterpret_cast<void*>(ptr);
    }

    void deallocate(void*, std::size_t) {}

    void reset() {
        deallocate_blocks(nullptr);
        state_.block_head_ = nullptr;
        state_.block_size_ = block_size_;
        push_block(block_initial_);
    }
};

template <typename T> struct arena_allocator_alignment {
    static constexpr std::size_t value = alignof(
        std::conditional_t< std::is_same_v<T, void>, std::max_align_t, T >
    );
};

template <typename T> inline constexpr std::size_t arena_allocator_alignment_v = arena_allocator_alignment<T>::value;

template <
    typename T,
    typename Arena = arena<>,
    std::size_t Alignment = alignof(std::max_align_t)
> class arena_allocator {
    static_assert((Alignment & (Alignment - 1)) == 0);

    template <typename U, typename ArenaU, std::size_t AlignmentU> friend class arena_allocator;
    Arena* arena_ = nullptr;

public:
    using value_type    = T;
    static constexpr std::size_t alignment = std::max(Alignment, arena_allocator_alignment_v<T>);

    template< typename U > struct rebind { using other = arena_allocator< U, Arena, Alignment >; };

    arena_allocator(Arena& arena) noexcept
        : arena_(&arena) {}

    template <typename U, std::size_t AlignmentU> arena_allocator(const arena_allocator<U, Arena, AlignmentU>& other) noexcept
        : arena_(other.arena_) {}

    value_type* allocate(std::size_t n) {
        if (std::numeric_limits<intptr_t>::max() / sizeof(T) < n)
            return nullptr;

        static_assert(alignment >= alignof(T));
        return reinterpret_cast<value_type*>(arena_->allocate(sizeof(T) * n, alignment));
    }

    void deallocate(value_type* ptr, std::size_t n) noexcept {
        arena_->deallocate(ptr, sizeof(T) * n);
    }
};

template < typename Arena, std::size_t Alignment > class arena_allocator<void, Arena, Alignment> {
    template <typename U, typename ArenaU, std::size_t AlignmentU> friend class arena_allocator;
    Arena* arena_ = nullptr;

public:
    using value_type = void;
    template< typename U > struct rebind { using other = arena_allocator< U, Arena, Alignment >; };

    arena_allocator(Arena& arena) noexcept
        : arena_(&arena) {}

    template <typename U, std::size_t AlignmentU> arena_allocator(const arena_allocator<U, Arena, AlignmentU>& other) noexcept
        : arena_(other.arena_) {}
};

template <typename T, std::size_t AlignmentT, typename U, std::size_t AlignmentU, typename Arena>
bool operator == (const arena_allocator<T, Arena, AlignmentT>& lhs, const arena_allocator<U, Arena, AlignmentU>& rhs) noexcept {
    return lhs.arena_ == rhs.arena_;
}

template <typename T, std::size_t AlignmentT, typename U, std::size_t AlignmentU, typename Arena>
bool operator != (const arena_allocator<T, Arena, AlignmentT>& x, const arena_allocator<U, Arena, AlignmentU>& y) noexcept {
    return !(x == y);
}

}


// ==============================================================================
// ==  resolution  —  type resolution, conversion cache & recursion guards
// ==============================================================================

// --- resolution/resolving_frame_fwd.h ---

export namespace silicon::di {

class resolving_frame;

} // export namespace silicon::di

// --- resolution/resolving_frame.h ---


export namespace silicon::di {


class context_path_state;

class resolving_frame {
  public:
    resolving_frame(context_path_state&, type_descriptor);

    resolving_frame(const resolving_frame&) = delete;
    resolving_frame& operator=(const resolving_frame&) = delete;
    resolving_frame(resolving_frame&&) = delete;
    resolving_frame& operator=(resolving_frame&&) = delete;

    ~resolving_frame();

  private:
    friend class context_path_state;

    context_path_state* context_;
    resolving_frame* parent_ = nullptr;
    type_descriptor type_;
};



} // export namespace silicon::di


// ==============================================================================
// ==  core  —  binding model, binding collection & context
// ==============================================================================

// --- core/context_base.h ---




export namespace silicon::di {


struct context_destructible {
    void* instance;
    void (*dtor)(void*);
};

// context_closure 接口：类型擦除门面
PRO_DEF_MEM_DISPATCH(MemClosureReset, reset);
PRO_DEF_MEM_DISPATCH(MemClosureArena, arena_storage);
PRO_DEF_MEM_DISPATCH(MemClosureAddDtor, add_destructor);

struct context_closure_facade
    : silicon::proxy::facade_builder
      ::add_convention<MemClosureReset, void()>
      ::add_convention<MemClosureArena, arena<>&()>
      ::add_convention<MemClosureAddDtor, void(void*, void (*)(void*))>
      ::build {};

using context_closure_proxy = silicon::proxy::proxy<context_closure_facade>;
using context_closure_view = silicon::proxy::proxy_view<context_closure_facade>;

template<class T, class... Args>
[[nodiscard]] context_closure_proxy make_context_closure(Args &&...args) {
    return silicon::proxy::make_proxy<context_closure_facade, T>(std::forward<Args>(args)...);
}

// 适配器：把具体 closure 的 do_* 实现桥接到 context_closure_facade
template<class Closure>
struct closure_strategy {
    Closure* self;
    void reset() { self->do_reset(); }
    arena<>& arena_storage() { return self->do_arena_storage(); }
    void add_destructor(void* instance, void (*dtor)(void*)) { self->do_add_destructor(instance, dtor); }
};

// 保留 context_closure_base 作为闭包节点类型（被 closures_ 以 context_closure_base* 持有，
// 多态调用 reset()/arena_storage()/add_destructor()），内部以类型擦除 strategy_ 承载具体实现。
struct context_closure_base {
    context_closure_proxy strategy_{};
    void reset() { strategy_->reset(); }
    arena<>& arena_storage() { return strategy_->arena_storage(); }
    void add_destructor(void* instance, void (*dtor)(void*)) { strategy_->add_destructor(instance, dtor); }
};

struct context_closure : context_closure_base {
    context_closure()
        : arena_(arena_buffer_)
        , destructibles_(arena_) {
        // 在构造函数体内（complete-class 上下文）初始化 strategy_，
        // 此时 do_* 已声明，closure_strategy<context_closure> 可被完整实例化。
        strategy_ = make_context_closure<closure_strategy<context_closure>>(this);
    }

    ~context_closure() { reset(); }

    context_closure(const context_closure&) = delete;
    context_closure& operator=(const context_closure&) = delete;

    void do_reset() {
        if (!destructibles_.empty()) {
            for (auto it = destructibles_.rbegin(); it != destructibles_.rend();
                 ++it) {
                it->dtor(it->instance);
            }
        }
        destructibles_.clear();
        // `destructibles_` stores its capacity inside `arena_`. Rebuild the
        // vector before rewinding the arena so the vector destructor never
        // observes capacity backed by invalidated storage.
        using destructibles_type = decltype(destructibles_);
        destructibles_.~destructibles_type();
        arena_.reset();
        new (&destructibles_) destructibles_type(arena_);
    }

    aligned_storage_t<SILICON_DI_CLOSURE_ARENA_BUFFER_SIZE,
                      alignof(std::max_align_t)>
        arena_buffer_;
    arena<> arena_;
    std::vector<context_destructible, arena_allocator<context_destructible>>
        destructibles_;

    arena<>& do_arena_storage() { return arena_; }

    void do_add_destructor(void* instance, void (*dtor)(void*)) {
        destructibles_.push_back({instance, dtor});
    }
};

template <std::size_t DestructibleCapacity,
          std::size_t TemporarySlotCapacity = 0,
          std::size_t TemporarySlotSize = 1,
          std::size_t TemporarySlotAlign = alignof(std::max_align_t)>
struct static_context_closure {
    static constexpr std::size_t destructible_capacity_ = DestructibleCapacity;
    static constexpr std::size_t temporary_slot_capacity_ =
        TemporarySlotCapacity;
    static constexpr std::size_t temporary_storage_capacity_ =
        temporary_slot_capacity_ == 0 ? 1 : temporary_slot_capacity_;

    static_context_closure() = default;
    ~static_context_closure() { reset(); }

    static_context_closure(const static_context_closure&) = delete;
    static_context_closure& operator=(const static_context_closure&) = delete;

    void reset() {
        while (destructible_count_ != 0) {
            auto& destructible = destructibles_[--destructible_count_];
            destructible.dtor(destructible.instance);
        }
        temporary_count_ = 0;
    }

    void add_destructor(void* instance, void (*dtor)(void*)) {
        assert(destructible_count_ < destructible_capacity_);
        destructibles_[destructible_count_++] = {instance, dtor};
    }

    template <typename T>
    T* try_allocate_temporary() {
        if constexpr (temporary_slot_capacity_ == 0) {
            return nullptr;
        } else if constexpr (sizeof(T) > TemporarySlotSize ||
                             alignof(T) > TemporarySlotAlign) {
            return nullptr;
        } else {
            if (temporary_count_ >= temporary_slot_capacity_) {
                return nullptr;
            }
            return reinterpret_cast<T*>(&temporary_slots_[temporary_count_++]);
        }
    }

    std::array<context_destructible, destructible_capacity_> destructibles_{};
    std::size_t destructible_count_ = 0;
    std::array<aligned_storage_t<TemporarySlotSize, TemporarySlotAlign>,
               temporary_storage_capacity_>
        temporary_slots_{};
    std::size_t temporary_count_ = 0;
};

template <std::size_t DestructibleCapacity, std::size_t TemporarySlotCapacity = 0,
          std::size_t TemporarySlotSize = 1,
          std::size_t TemporarySlotAlign = alignof(std::max_align_t)>
struct fixed_context_closure : context_closure_base {
    static constexpr std::size_t destructible_capacity_ = DestructibleCapacity;
    static constexpr std::size_t temporary_slot_capacity_ =
        TemporarySlotCapacity;
    static constexpr std::size_t temporary_storage_capacity_ =
        temporary_slot_capacity_ == 0 ? 1 : temporary_slot_capacity_;

    fixed_context_closure()
        : arena_(arena_buffer_) {
        strategy_ = make_context_closure<closure_strategy<fixed_context_closure>>(this);
    }

    ~fixed_context_closure() { reset(); }

    fixed_context_closure(const fixed_context_closure&) = delete;
    fixed_context_closure& operator=(const fixed_context_closure&) = delete;

    void do_reset() {
        while (destructible_count_ != 0) {
            auto& destructible = destructibles_[--destructible_count_];
            destructible.dtor(destructible.instance);
        }
        temporary_count_ = 0;
        arena_.reset();
    }

    arena<>& do_arena_storage() { return arena_; }

    void do_add_destructor(void* instance, void (*dtor)(void*)) {
        assert(destructible_count_ < destructible_capacity_);
        destructibles_[destructible_count_++] = {instance, dtor};
    }

    template <typename T>
    T* try_allocate_temporary() {
        if constexpr (temporary_slot_capacity_ == 0) {
            return nullptr;
        } else if constexpr (sizeof(T) > TemporarySlotSize ||
                             alignof(T) > TemporarySlotAlign) {
            return nullptr;
        } else {
            if (temporary_count_ >= temporary_slot_capacity_) {
                return nullptr;
            }
            return reinterpret_cast<T*>(&temporary_slots_[temporary_count_++]);
        }
    }

    aligned_storage_t<SILICON_DI_CLOSURE_ARENA_BUFFER_SIZE,
                      alignof(std::max_align_t)>
        arena_buffer_;
    arena<> arena_;
    std::array<context_destructible, destructible_capacity_> destructibles_{};
    std::size_t destructible_count_ = 0;
    std::array<aligned_storage_t<TemporarySlotSize, TemporarySlotAlign>,
               temporary_storage_capacity_>
        temporary_slots_{};
    std::size_t temporary_count_ = 0;
};

class context_path_state {
  public:
    template <typename T>
    resolving_frame track_type();

    bool has_type_path() const;

    const type_descriptor* active_type() const;

    const type_descriptor* parent_type() const;

    void append_type_path(std::string&) const;

  protected:
    friend class resolving_frame;

    resolving_frame* active_resolving_frame_ = nullptr;
};

class context_state : public context_path_state {
  public:
    context_state()
        : arena_(arena_buffer_)
        , closures_(arena_) {
        closures_.emplace_back(&closure_);
    }

    ~context_state() {
        for (auto it = closures_.rbegin(); it != closures_.rend(); ++it) {
            (*it)->reset();
        }
    }

    template <typename T, typename... Args>
    T& construct(Args&&... args) {
        arena_allocator<void> alloc(closures_.back()->arena_storage());
        auto allocator = allocator_traits::rebind<T>(alloc);
        auto instance = allocator_traits::allocate(allocator, 1);
        allocator_traits::construct(allocator, instance,
                                    std::forward<Args>(args)...);
        if constexpr (!std::is_trivially_destructible_v<T>) {
            register_destructor(instance);
        }
        return *instance;
    }

    template <typename T>
    T* allocate() {
        arena_allocator<void> alloc(closures_.back()->arena_storage());
        auto allocator = allocator_traits::rebind<T>(alloc);
        return allocator_traits::allocate(allocator, 1);
    }

    void push(context_closure_base* c) {
        // A closure represents one owner of preserved temporaries. Recursive
        // shared resolution must be stopped by the type guard before the same
        // factory tries to reactivate its closure while it is already active.
        assert(!contains(c));
        closures_.emplace_back(c);
    }

    void pop() { closures_.pop_back(); }

    bool contains(const context_closure_base* candidate) const {
        for (auto* active : closures_) {
            if (active == candidate) {
                return true;
            }
        }
        return false;
    }

  protected:
    template <typename T>
    void register_destructor(T* instance) {
        static_assert(!std::is_trivially_destructible_v<T>);
        closures_.back()->add_destructor(instance, &destructor<T>);
    }

    template <typename T>
    static void destructor(void* ptr) {
        reinterpret_cast<T*>(ptr)->~T();
    }

    aligned_storage_t<SILICON_DI_CONTEXT_ARENA_BUFFER_SIZE,
                      alignof(std::max_align_t)>
        arena_buffer_;
    arena<> arena_;
    std::vector<context_closure_base*, arena_allocator<context_closure_base*>>
        closures_;
    context_closure closure_;
};


} // export namespace silicon::di


export namespace silicon::di {


#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdangling-pointer"
#endif
inline resolving_frame::resolving_frame(context_path_state& context,
                                        type_descriptor type)
    : context_(&context)
    , parent_(context.active_resolving_frame_)
    , type_(type) {
    // The frame object itself is the linked stack node for the active
    // resolution path, so entering the scope just makes this the new head.
    // GCC can warn here when the frame lives inside a stack RAII guard even
    // though the destructor restores parent_ before that guard leaves scope.
    context_->active_resolving_frame_ = this;
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

inline resolving_frame::~resolving_frame() {
    if (context_) {
        // Frames unwind strictly LIFO; restoring parent_ pops this node from
        // the active type path without touching any arena-backed state.
        assert(context_->active_resolving_frame_ == this);
        context_->active_resolving_frame_ = parent_;
    }
}

inline bool context_path_state::has_type_path() const {
    return active_resolving_frame_ != nullptr;
}

inline const type_descriptor* context_path_state::active_type() const {
    return active_resolving_frame_ != nullptr
               ? &active_resolving_frame_->type_
               : nullptr;
}

inline const type_descriptor* context_path_state::parent_type() const {
    return active_resolving_frame_ != nullptr &&
                   active_resolving_frame_->parent_ != nullptr
               ? &active_resolving_frame_->parent_->type_
               : nullptr;
}

inline void context_path_state::append_type_path(std::string& message) const {
    std::vector<type_descriptor> names;
    for (auto* frame = active_resolving_frame_; frame != nullptr;
         frame = frame->parent_) {
        names.emplace_back(frame->type_);
    }

    for (auto it = names.rbegin(); it != names.rend(); ++it) {
        if (it != names.rbegin()) {
            message += " -> ";
        }
        append_type_name(message, *it);
    }
}

template <typename T>
resolving_frame context_path_state::track_type() {
    return resolving_frame(*this, describe_type<T>());
}


} // export namespace silicon::di


// ==============================================================================
// ==  type  —  type traits, descriptors, type lists & maps
// ==============================================================================

// --- type/type_conversion_traits.h ---



export namespace silicon::di {
template <typename TargetType, typename SourceType, typename = void>
struct type_conversion_traits {
    template <typename Source>
    static TargetType convert(Source&& source) {
        static_assert(
            std::is_constructible_v<TargetType, Source&&>,
            "type conversion requires a type_conversion_traits "
            "specialization or a direct converting constructor");
        return TargetType(std::forward<Source>(source));
    }
};

template <typename Target, typename Source>
struct type_conversion_traits<Target*, Source*> {
    static Target* convert(Source* source) { return static_cast<Target*>(source); }
};
} // export namespace silicon::di


// ==============================================================================
// ==  resolution  —  type resolution, conversion cache & recursion guards
// ==============================================================================

// --- resolution/conversion_cache.h ---




export namespace silicon::di {

template <typename T, typename... Args>
void construct_class_instance(void* ptr, Args&&... args) {
    new (ptr) T(std::forward<Args>(args)...);
}

template <typename T, typename Source>
std::enable_if_t<type_traits<T>::enabled &&
                 type_traits<std::remove_reference_t<Source>>::enabled>
construct_class_instance(void* ptr, Source&& source) {
    new (ptr) T(type_conversion_traits<
                T, std::remove_reference_t<Source>>::convert(
        std::forward<Source>(source)));
}


template <typename... Types> struct conversion_cache;

template <> struct conversion_cache<> {
    void reset() {}
};

template <typename T> struct conversion_cache_entry {
    template <typename... Args> T& construct(Args&&... args) {
        auto* instance = reinterpret_cast<T*>(&storage_);
        if (initialized_) {
            return *instance;
        }

        construct_class_instance<T>(instance,
                                            std::forward<Args>(args)...);
        initialized_ = true;
        return *instance;
    }

    void reset() {
        if (initialized_) {
            if constexpr (!std::is_trivially_destructible_v<T>)
                reinterpret_cast<T*>(&storage_)->~T();
            initialized_ = false;
        }
    }

    aligned_storage_t<sizeof(T), alignof(T)> storage_;
    bool initialized_ = false;
};

template <typename... Types> struct conversion_cache
    : conversion_cache_entry<Types>...
{
    ~conversion_cache() {
        reset();
    }

    template <typename T, typename... Args> T& construct(Args&&... args) {
        return static_cast<conversion_cache_entry<T>&>(*this).construct(
            std::forward<Args>(args)...);
    }

    void reset() {
        (static_cast<conversion_cache_entry<Types>&>(*this).reset(), ...);
    }
};

// TODO: the idea was that conversions will act as a cache of pointers
// context-owned instances. That would require pushing and popping of
// the context on each resolution. Right now, conversions are like
// "expanded variant".
template <typename... Args> struct conversion_cache<type_list<Args...>>
    : conversion_cache<Args...>
{}; 

template <> struct conversion_cache<type_list<>> {
    void reset() {}
};

} // export namespace silicon::di

// --- resolution/runtime_binding_interface.h ---


export namespace silicon::di {
class runtime_context;

template <typename RTTI> struct instance_request {
    using type_index = typename RTTI::type_index;

    type_index lookup_type;
    type_descriptor requested_type;
};

template <typename T> struct request_lookup_type {
    using type = rebind_leaf_t<T, runtime_type>;
};

template <typename T> struct request_lookup_type<const T&> {
    using type = rebind_leaf_t<T&, runtime_type>;
};

template <typename T>
using request_lookup_type_t = typename request_lookup_type<T>::type;

struct instance_cache_sink {
    void* context = nullptr;
    void (*store)(void*, void*) = nullptr;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
    void operator()(void* ptr) const {
        if (store) {
            store(context, ptr);
        }
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

template <typename Container> class runtime_binding_interface {
  public:
    virtual ~runtime_binding_interface() = default;

    virtual void* get_value(
        runtime_context&,
        const instance_request<typename Container::rtti_type>& request,
        instance_cache_sink) = 0;
    virtual void* get_lvalue_reference(
        runtime_context&,
        const instance_request<typename Container::rtti_type>& request,
        instance_cache_sink) = 0;
    virtual void* get_rvalue_reference(
        runtime_context&,
        const instance_request<typename Container::rtti_type>& request,
        instance_cache_sink) = 0;
    virtual void* get_pointer(
        runtime_context&,
        const instance_request<typename Container::rtti_type>& request,
        instance_cache_sink) = 0;

    virtual void destroy() = 0;
};
} // export namespace silicon::di


// ==============================================================================
// ==  memory  —  allocators, arena & object lifetime
// ==============================================================================

// --- memory/object_lifetime.h ---



export namespace silicon::di {

template <typename Type> void destroy_object_value(Type& value) {
    if constexpr (std::is_array_v<Type>) {
        for (std::size_t i = std::extent_v<Type>; i > 0; --i) {
            destroy_object_value(value[i - 1]);
        }
    } else if constexpr (!std::is_trivially_destructible_v<Type>) {
        value.~Type();
    }
}

} // export namespace silicon::di


// ==============================================================================
// ==  storage  —  storage policies (shared / unique / external / cyclical)
// ==============================================================================

// --- storage/materialized_source.h ---



#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

export namespace silicon::di {

template <typename Source> class rvalue_source {
  public:
    using value_type = Source;

    template <typename Value,
              typename = std::enable_if_t<
                  std::is_constructible_v<Source, Value&&> &&
                  !std::is_same_v<std::remove_cv_t<std::remove_reference_t<Value>>,
                                  rvalue_source>>>
    explicit rvalue_source(Value&& value) {
        new (&storage_) Source(std::forward<Value>(value));
        initialized_ = true;
    }

    template <typename Builder>
    explicit rvalue_source(std::in_place_t, Builder&& builder) {
        builder(&storage_);
        initialized_ = true;
    }

    rvalue_source(const rvalue_source&) = delete;
    rvalue_source& operator=(const rvalue_source&) = delete;
    rvalue_source(rvalue_source&&) = delete;
    rvalue_source& operator=(rvalue_source&&) = delete;

    ~rvalue_source() {
        if (initialized_) {
            destroy_object_value(get());
        }
    }

    Source* get_ptr() {
        return std::launder(reinterpret_cast<Source*>(&storage_));
    }
    const Source* get_ptr() const {
        return std::launder(reinterpret_cast<const Source*>(&storage_));
    }

    Source& get() & { return *get_ptr(); }
    const Source& get() const & { return *get_ptr(); }
    Source get() && { return std::move(*get_ptr()); }

    Source& operator*() { return get(); }
    const Source& operator*() const { return get(); }

    Source* operator->() { return get_ptr(); }
    const Source* operator->() const { return get_ptr(); }

  private:
    alignas(Source) std::byte storage_[sizeof(Source)];
    bool initialized_ = false;
};

template <typename Source> struct lvalue_source {
    using value_type = Source;

    explicit lvalue_source(Source& source) : value_(std::addressof(source)) {}

    Source& get() { return *value_; }
    const Source& get() const { return *value_; }

    Source* get_ptr() { return value_; }
    const Source* get_ptr() const { return value_; }

    Source& operator*() { return get(); }
    const Source& operator*() const { return get(); }

    Source* operator->() { return get_ptr(); }
    const Source* operator->() const { return get_ptr(); }

  private:
    Source* value_;
};

template <typename Source> struct pointer_source {
    using value_type = Source;

    explicit pointer_source(Source* source) : value_(source) {}

    Source* get() { return value_; }
    const Source* get() const { return value_; }

    Source& operator*() { return *value_; }
    const Source& operator*() const { return *value_; }

    Source* operator->() { return value_; }
    const Source* operator->() const { return value_; }

  private:
    Source* value_;
};

template <typename SourceCapability> struct materialized_source_traits;

template <typename Source>
struct materialized_source_traits<rvalue_source<Source>> {
    using value_type = Source;
    static constexpr bool reference_like = false;

    template <typename Capability>
    static decltype(auto) value(Capability&& source) {
        return std::forward<Capability>(source).get();
    }

    static Source& reference(rvalue_source<Source>& source) {
        return source.get();
    }

    static const Source& reference(const rvalue_source<Source>& source) {
        return source.get();
    }

    static Source* pointer(rvalue_source<Source>& source) {
        return source.get_ptr();
    }

    static const Source* pointer(const rvalue_source<Source>& source) {
        return source.get_ptr();
    }
};

template <typename Source>
struct materialized_source_traits<lvalue_source<Source>> {
    using value_type = Source;
    static constexpr bool reference_like = true;

    template <typename Capability>
    static decltype(auto) value(Capability&& source) {
        return std::forward<Capability>(source).get();
    }

    static Source& reference(lvalue_source<Source>& source) {
        return source.get();
    }

    static const Source& reference(const lvalue_source<Source>& source) {
        return source.get();
    }

    static Source* pointer(lvalue_source<Source>& source) {
        return source.get_ptr();
    }

    static const Source* pointer(const lvalue_source<Source>& source) {
        return source.get_ptr();
    }
};

template <typename Source>
struct materialized_source_traits<pointer_source<Source>> {
    using value_type = Source;
    static constexpr bool reference_like = true;

    template <typename Capability>
    static decltype(auto) value(Capability&& source) {
        return std::forward<Capability>(source).get();
    }

    static Source& reference(pointer_source<Source>& source) {
        return *source.get();
    }

    static const Source& reference(const pointer_source<Source>& source) {
        return *source.get();
    }

    static Source* pointer(pointer_source<Source>& source) {
        return source.get();
    }

    static const Source* pointer(const pointer_source<Source>& source) {
        return source.get();
    }
};

template <typename Source, typename Builder>
rvalue_source<Source> make_rvalue_source(std::in_place_t, Builder&& builder) {
    return rvalue_source<Source>(std::in_place, std::forward<Builder>(builder));
}

template <typename Source>
auto make_rvalue_source(Source&& source)
    -> rvalue_source<std::remove_cv_t<std::remove_reference_t<Source>>> {
    using value_type = std::remove_cv_t<std::remove_reference_t<Source>>;
    return rvalue_source<value_type>(std::forward<Source>(source));
}

template <typename Source>
lvalue_source<Source> make_lvalue_source(Source& source) {
    return lvalue_source<Source>(source);
}

template <typename Source>
pointer_source<Source> make_pointer_source(Source* source) {
    return pointer_source<Source>(source);
}

template <typename Source>
lvalue_source<Source> make_resolved_source(Source& source) {
    return make_lvalue_source(source);
}

template <typename Source>
pointer_source<Source> make_resolved_source(Source* source) {
    return make_pointer_source(source);
}


} // export namespace silicon::di

#ifdef _MSC_VER
#pragma warning(pop)
#endif


// ==============================================================================
// ==  resolution  —  type resolution, conversion cache & recursion guards
// ==============================================================================

// --- resolution/type_conversion.h ---




#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

export namespace silicon::di {

template <typename Target, typename Source>
inline constexpr bool is_same_handle_shape_v =
    std::is_same_v<rebind_leaf_t<Target, runtime_type>,
                   rebind_leaf_t<Source, runtime_type>>;

template <typename Target, typename Source>
inline constexpr bool is_rebindable_handle_v =
    // Identical handle types should reuse the already materialized source
    // instead of re-entering factory.resolve<Target>(), which can re-trigger
    // shared recursion handling while the source instance is still resolving.
    !std::is_same_v<Target, Source> && is_same_handle_shape_v<Target, Source> &&
    type_traits<Source>::enabled && type_traits<Source>::is_pointer_like &&
    type_traits<Source>::template is_rebindable<Target>;

template <typename SourceCapability>
using materialized_source_traits_t = materialized_source_traits<
    std::remove_cv_t<std::remove_reference_t<SourceCapability>>>;

template <typename SourceCapability>
using source_value_type_t =
    typename materialized_source_traits_t<SourceCapability>::value_type;

template <typename Type>
using unqualified_t = std::remove_cv_t<std::remove_reference_t<Type>>;

template <typename SourceCapability>
decltype(auto) materialized_value(SourceCapability&& source) {
    return materialized_source_traits_t<SourceCapability>::value(
        std::forward<SourceCapability>(source));
}

template <typename SourceCapability>
decltype(auto) materialized_reference(SourceCapability&& source) {
    return materialized_source_traits_t<SourceCapability>::reference(source);
}

template <typename SourceCapability>
decltype(auto) materialized_pointer(SourceCapability&& source) {
    return materialized_source_traits_t<SourceCapability>::pointer(source);
}

template <typename Target, typename Source>
std::expected<std::reference_wrapper<Target>, std::error_code>
borrow_reference(Source& source, type_descriptor requested_type,
                 type_descriptor registered_type) {
    if constexpr (std::is_same_v<Target, Source>) {
        return source;
    } else if constexpr (std::is_convertible_v<Source*, Target*>) {
        return static_cast<Target&>(source);
    } else if constexpr (type_traits<Source>::enabled &&
                         type_traits<Source>::is_value_borrowable) {
        auto borrowed = borrow_reference<Target>(
                type_traits<Source>::borrow(source),
                requested_type, registered_type);
        if (!borrowed)
            return std::unexpected(borrowed.error());
        return *borrowed;
    } else {
        return std::unexpected(make_type_not_convertible_exception(
                requested_type, registered_type));
    }
}

template <typename Target, typename Source>
std::expected<Target*, std::error_code>
borrow_pointer(Source& source, type_descriptor requested_type,
               type_descriptor registered_type) {
    if constexpr (std::is_convertible_v<Source*, Target*>) {
        return static_cast<Target*>(&source);
    } else if constexpr (type_traits<Source>::enabled &&
                         std::is_convertible_v<decltype(type_traits<Source>::get(
                                                   source)),
                                                Target*>) {
        return type_traits<Source>::get(source);
    } else if constexpr (type_traits<Source>::enabled &&
                         type_traits<Source>::is_value_borrowable) {
        auto borrowed = borrow_pointer<Target>(
                type_traits<Source>::borrow(source),
                requested_type, registered_type);
        if (!borrowed)
            return std::unexpected(borrowed.error());
        return *borrowed;
    } else {
        return std::unexpected(make_type_not_convertible_exception(
                requested_type, registered_type));
    }
}

template <typename Target, typename Source, typename Factory, typename Context,
          typename SourceCapability>
std::expected<std::reference_wrapper<Target>, std::error_code>
resolve_handle_or_borrow(Factory& factory, Context& context,
                         SourceCapability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
    using materialized_reference_type =
        decltype(materialized_reference(std::declval<SourceCapability>()));
    if constexpr (std::is_constructible_v<Target,
                                          materialized_reference_type>) {
        return factory.template resolve_conversion<Target>(
            context, materialized_reference(source));
    } else if constexpr (is_rebindable_handle_v<Target, Source>) {
        auto resolved = type_traits<Source>::template resolve_type<Target>(
            factory, context, requested_type, registered_type);
        if (!resolved)
            return std::unexpected(resolved.error());
        return *resolved;
    } else {
        auto borrowed = borrow_reference<Target>(materialized_value(
                                            std::forward<SourceCapability>(
                                                source)),
                                        requested_type, registered_type);
        if (!borrowed)
            return std::unexpected(borrowed.error());
        return *borrowed;
    }
}

template <typename Target, typename SourceCapability>
std::expected<std::reference_wrapper<Target>, std::error_code>
resolve_materialized_convertible_reference(
    SourceCapability&& source, type_descriptor requested_type,
    type_descriptor registered_type) {
    using source_type = source_value_type_t<SourceCapability>;
    if constexpr (std::is_convertible_v<source_type*, Target*>) {
        return static_cast<Target&>(materialized_reference(source));
    } else {
        return std::unexpected(make_type_not_convertible_exception(
                requested_type, registered_type));
    }
}

template <typename Target, typename SourceCapability>
std::expected<Target*, std::error_code>
resolve_materialized_convertible_pointer(
    SourceCapability&& source, type_descriptor requested_type,
    type_descriptor registered_type) {
    auto resolved = resolve_materialized_convertible_reference<Target>(
        std::forward<SourceCapability>(source), requested_type,
        registered_type);
    if (!resolved)
        return std::unexpected(resolved.error());
    return std::addressof(resolved->get());
}

template <typename Target, typename SourceCapability>
Target resolve_materialized_handle_from_pointer(SourceCapability&& source) {
    return type_traits<Target>::from_pointer(
        materialized_value(std::forward<SourceCapability>(source)));
}

template <typename Target, typename SourceCapability>
Target& resolve_borrowed_materialized_reference(
    SourceCapability&& source, type_descriptor requested_type,
    type_descriptor registered_type) {
    return borrow_reference<Target>(materialized_reference(source),
                                    requested_type, registered_type);
}

template <typename Target, typename SourceCapability>
Target* resolve_borrowed_materialized_pointer(
    SourceCapability&& source, type_descriptor requested_type,
    type_descriptor registered_type) {
    return borrow_pointer<Target>(materialized_reference(source), requested_type,
                                  registered_type);
}

template <typename Target, typename Source, typename Factory, typename Context,
          typename SourceCapability>
std::expected<Target*, std::error_code>
resolve_handle_or_borrow_pointer(Factory& factory, Context& context,
                                 SourceCapability&& source,
                                 type_descriptor requested_type,
                                 type_descriptor registered_type) {
    using materialized_reference_type =
        decltype(materialized_reference(std::declval<SourceCapability>()));
    if constexpr (std::is_constructible_v<Target,
                                          materialized_reference_type>) {
        return std::addressof(factory.template resolve_conversion<Target>(
            context, materialized_reference(source)));
    } else if constexpr (is_rebindable_handle_v<Target, Source>) {
        auto resolved = type_traits<Source>::template resolve_type<Target>(
            factory, context, requested_type, registered_type);
        if (!resolved)
            return std::unexpected(resolved.error());
        return std::addressof(resolved->get());
    } else {
        auto borrowed = borrow_reference<Target>(
            materialized_value(std::forward<SourceCapability>(source)),
            requested_type, registered_type);
        if (!borrowed)
            return std::unexpected(borrowed.error());
        return std::addressof(borrowed->get());
    }
}

template <typename Target, typename Source, typename SourceCapability>
std::expected<Target*, std::error_code>
resolve_materialized_get_pointer(SourceCapability&& source,
                                 type_descriptor requested_type,
                                 type_descriptor registered_type) {
    auto& value = materialized_reference(source);
    if constexpr (std::is_convertible_v<decltype(type_traits<Source>::get(value)),
                                        Target*>) {
        return type_traits<Source>::get(value);
    } else {
        return std::unexpected(make_type_not_convertible_exception(
                requested_type, registered_type));
    }
}

template <typename Target, typename Sum>
std::expected<Target, std::error_code>
extract_alternative_type_value(Sum&& sum, type_descriptor requested_type,
                               type_descriptor registered_type) {
    using selected_type = unqualified_t<Target>;
    using alternative_type = unqualified_t<Sum>;

    if (auto* value =
            alternative_type_traits<alternative_type>::template get<selected_type>(
                sum)) {
        return Target(std::move(*value));
    }

    return std::unexpected(make_type_not_convertible_exception(
            requested_type, registered_type));
}

template <typename Target, typename Sum>
std::expected<std::reference_wrapper<Target>, std::error_code>
resolve_alternative_type_reference(Sum& sum,
                                   type_descriptor requested_type,
                                   type_descriptor registered_type) {
    using selected_type = unqualified_t<Target>;
    using alternative_type = unqualified_t<Sum>;

    if (auto* value =
            alternative_type_traits<alternative_type>::template get<selected_type>(
                sum)) {
        return *value;
    }

    return std::unexpected(make_type_not_convertible_exception(
            requested_type, registered_type));
}

template <typename Target, typename Sum>
std::expected<Target*, std::error_code>
resolve_alternative_type_pointer(Sum& sum,
                                 type_descriptor requested_type,
                                 type_descriptor registered_type) {
    auto resolved = resolve_alternative_type_reference<Target>(
        sum, requested_type, registered_type);
    if (!resolved)
        return std::unexpected(resolved.error());
    return std::addressof(resolved->get());
}

template <typename Target, typename Sum, typename Alternatives>
struct alternative_borrow_reference;

template <typename Target, typename Sum>
struct alternative_borrow_reference<Target, Sum, type_list<>> {
    static std::expected<std::reference_wrapper<Target>, std::error_code>
    resolve(Sum&, type_descriptor requested_type,
            type_descriptor registered_type) {
        return std::unexpected(make_type_not_convertible_exception(
                requested_type, registered_type));
    }
};

template <typename Target, typename Sum, typename Alternative,
          typename... Alternatives>
struct alternative_borrow_reference<Target, Sum,
                                    type_list<Alternative, Alternatives...>> {
    static std::expected<std::reference_wrapper<Target>, std::error_code>
    resolve(Sum& sum, type_descriptor requested_type,
            type_descriptor registered_type) {
        using alternative_type = unqualified_t<Sum>;
        if (auto* value =
                alternative_type_traits<alternative_type>::template get<
                    Alternative>(sum)) {
            auto borrowed = borrow_reference<Target>(*value, requested_type,
                                                    registered_type);
            if (!borrowed)
                return std::unexpected(borrowed.error());
            return *borrowed;
        }

        return alternative_borrow_reference<
            Target, Sum, type_list<Alternatives...>>::resolve(
            sum, requested_type, registered_type);
    }
};

template <typename Target, typename Sum>
std::expected<std::reference_wrapper<Target>, std::error_code>
resolve_alternative_borrowed_reference(
    Sum& sum, type_descriptor requested_type, type_descriptor registered_type) {
    using alternative_type = unqualified_t<Sum>;
    return alternative_borrow_reference<
        Target, Sum, alternative_type_alternatives_t<alternative_type>>::resolve(
        sum, requested_type, registered_type);
}

template <typename Target, typename Sum>
std::expected<Target*, std::error_code>
resolve_alternative_borrowed_pointer(
    Sum& sum, type_descriptor requested_type, type_descriptor registered_type) {
    auto resolved = resolve_alternative_borrowed_reference<Target>(
        sum, requested_type, registered_type);
    if (!resolved)
        return std::unexpected(resolved.error());
    return std::addressof(resolved->get());
}

template <typename Target, typename SourceCapability>
std::expected<Target, std::error_code>
resolve_materialized_alternative_value(
    SourceCapability&& source, type_descriptor requested_type,
    type_descriptor registered_type) {
    auto&& value = materialized_value(std::forward<SourceCapability>(source));
    return extract_alternative_type_value<Target>(std::move(value),
                                                  requested_type,
                                                  registered_type);
}

template <typename Target, typename SourceCapability>
std::expected<std::reference_wrapper<Target>, std::error_code>
resolve_materialized_alternative_reference(
    SourceCapability&& source, type_descriptor requested_type,
    type_descriptor registered_type) {
    return resolve_alternative_type_reference<Target>(
        materialized_reference(source), requested_type, registered_type);
}

template <typename Target, typename SourceCapability>
std::expected<Target*, std::error_code>
resolve_materialized_alternative_pointer(
    SourceCapability&& source, type_descriptor requested_type,
    type_descriptor registered_type) {
    return resolve_alternative_type_pointer<Target>(
        materialized_reference(source), requested_type, registered_type);
}

template <typename Source>
using borrowed_value_type_t = std::remove_cv_t<std::remove_reference_t<
    decltype(type_traits<Source>::borrow(std::declval<Source&>()))>>;

template <typename Source, typename Target, typename = void>
struct is_borrowed_alternative_type_alternative : std::false_type {};

template <typename Source, typename Target, typename = void>
struct is_borrowed_alternative_type : std::false_type {};

template <typename Source, typename Target>
struct is_borrowed_alternative_type<
    Source, Target,
    std::enable_if_t<type_traits<Source>::enabled &&
                     type_traits<Source>::is_value_borrowable &&
                     is_alternative_type_v<borrowed_value_type_t<Source>>>>
    : std::bool_constant<
          std::is_same_v<unqualified_t<Target>, borrowed_value_type_t<Source>>> {
};

template <typename Source, typename Target>
struct is_borrowed_alternative_type_alternative<
    Source, Target,
    std::enable_if_t<type_traits<Source>::enabled &&
                     type_traits<Source>::is_value_borrowable &&
                     is_alternative_type_v<borrowed_value_type_t<Source>>>>
    : std::bool_constant<
          !std::is_same_v<unqualified_t<Target>, borrowed_value_type_t<Source>> &&
          (alternative_type_count<borrowed_value_type_t<Source>,
                                  unqualified_t<Target>>::value == 1)> {};

template <typename Source, typename Target>
inline constexpr bool is_borrowed_alternative_type_v =
    is_borrowed_alternative_type<Source, Target>::value;

template <typename Source, typename Target>
inline constexpr bool is_borrowed_alternative_type_alternative_v =
    is_borrowed_alternative_type_alternative<Source, Target>::value;

template <typename SourceCapability>
decltype(auto) resolve_borrowed_materialized_value(SourceCapability&& source) {
    using source_type = source_value_type_t<SourceCapability>;
    auto& value = materialized_reference(source);
    return type_traits<source_type>::borrow(value);
}

template <typename Target, typename SourceCapability>
Target* resolve_borrowed_materialized_value_pointer(
    SourceCapability&& source) {
    return std::addressof(static_cast<Target&>(
        resolve_borrowed_materialized_value(
            std::forward<SourceCapability>(source))));
}

template <typename Target, typename SourceCapability>
Target& resolve_borrowed_materialized_alternative_reference(
    SourceCapability&& source, type_descriptor requested_type,
    type_descriptor registered_type) {
    return resolve_alternative_type_reference<Target>(
        resolve_borrowed_materialized_value(
            std::forward<SourceCapability>(source)),
        requested_type, registered_type);
}

template <typename Target, typename SourceCapability>
Target* resolve_borrowed_materialized_alternative_pointer(
    SourceCapability&& source, type_descriptor requested_type,
    type_descriptor registered_type) {
    return resolve_alternative_type_pointer<Target>(
        resolve_borrowed_materialized_value(
            std::forward<SourceCapability>(source)),
        requested_type, registered_type);
}


// TODO: this file is really terrible, I need to look at how to deduplicate it

template <typename Target, typename SourceCapability, typename = void>
struct type_conversion {
    template <typename Factory, typename Context, typename Source>
    static void* apply(Factory&, Context&, Source&&,
                       type_descriptor requested_type,
                       type_descriptor registered_type);
};

template <typename Target, typename Source>
struct type_conversion<Target, rvalue_source<Source>,
                       std::enable_if_t<!std::is_pointer_v<Source> &&
                                        !is_alternative_type_v<Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static decltype(auto) apply(Factory&, Context&, SourceCapability&& source,
                                type_descriptor, type_descriptor) {
        return std::move(*source.get_ptr());
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, rvalue_source<Source>,
    std::enable_if_t<
        is_alternative_type_v<Source> &&
        !std::is_pointer_v<Target> &&
        !std::is_same_v<unqualified_t<Target>,
                        unqualified_t<Source>> &&
        (alternative_type_count<Source,
                                        unqualified_t<Target>>::value == 1)>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target apply(Factory&, Context&, SourceCapability&& source,
                        type_descriptor requested_type,
                        type_descriptor registered_type) {
        return resolve_materialized_alternative_value<Target>(
            std::forward<SourceCapability>(source), requested_type,
            registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, rvalue_source<Source>,
    std::enable_if_t<is_alternative_type_v<Source> &&
                     std::is_same_v<unqualified_t<Target>,
                                    unqualified_t<Source>>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static decltype(auto) apply(Factory&, Context&, SourceCapability&& source,
                                type_descriptor, type_descriptor) {
        return std::move(*source.get_ptr());
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, rvalue_source<Source>,
    std::enable_if_t<
        is_alternative_type_v<Source> &&
        !std::is_pointer_v<Target> &&
        !std::is_same_v<unqualified_t<Target>,
                        unqualified_t<Source>> &&
        (alternative_type_count<Source,
                                        unqualified_t<Target>>::value != 1)>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static as_expected_t<Target> apply(Factory&, Context&, SourceCapability&&,
                        type_descriptor requested_type,
                        type_descriptor registered_type) {
        return std::unexpected(make_type_not_convertible_exception(
            requested_type, registered_type));
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, rvalue_source<Source*>,
    std::enable_if_t<std::is_array_v<Target> &&
                     std::is_same_v<std::remove_cv_t<Source>,
                                    std::remove_cv_t<Target>>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return materialized_value(std::forward<SourceCapability>(source));
    }
};

template <typename Target, size_t N, typename Source>
struct type_conversion<
    Target (*)[N], rvalue_source<Source*>,
    std::enable_if_t<std::is_same_v<std::remove_cv_t<Source>,
                                    std::remove_cv_t<Target>>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target (*apply(Factory&, Context&, SourceCapability&& source,
                          type_descriptor, type_descriptor))[N] {
        return reinterpret_cast<Target(*)[N]>(
            materialized_value(std::forward<SourceCapability>(source)));
    }
};

template <typename Target, typename Source>
struct type_conversion<Target*, rvalue_source<Source*>,
                       std::enable_if_t<!std::is_array_v<Target>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return materialized_value(std::forward<SourceCapability>(source));
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, rvalue_source<Source*>,
    std::enable_if_t<is_pointer_like_type_v<Target> &&
                     has_type_from_pointer_v<Target, Source*>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target apply(Factory&, Context&, SourceCapability&& source,
                        type_descriptor, type_descriptor) {
        return resolve_materialized_handle_from_pointer<Target>(
            std::forward<SourceCapability>(source));
    }
};

template <typename Array, typename Source, typename Deleter>
struct type_conversion<
    std::unique_ptr<Array, Deleter>, rvalue_source<Source*>,
    std::enable_if_t<(std::rank_v<Array> > 1) &&
                     (std::extent_v<Array, 0> != 0)>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static as_expected_t<std::unique_ptr<Array, Deleter>>
    apply(Factory&, Context&, SourceCapability&&, type_descriptor requested_type,
          type_descriptor registered_type) {
        return std::unexpected(make_type_not_convertible_exception(
            requested_type, registered_type));
    }
};

template <typename Array, typename Source>
struct type_conversion<std::shared_ptr<Array>, rvalue_source<Source*>,
                       std::enable_if_t<(std::rank_v<Array> > 1) &&
                                        (std::extent_v<Array, 0> != 0)>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static as_expected_t<std::shared_ptr<Array>>
    apply(Factory&, Context&, SourceCapability&&, type_descriptor requested_type,
          type_descriptor registered_type) {
        return std::unexpected(make_type_not_convertible_exception(
            requested_type, registered_type));
    }
};

template <typename Target, typename SourceCapability>
struct type_conversion<
    Target, SourceCapability,
    std::enable_if_t<
        materialized_source_traits_t<SourceCapability>::reference_like &&
        is_alternative_type_v<source_value_type_t<SourceCapability>> &&
        !std::is_pointer_v<Target> &&
        !std::is_same_v<
            unqualified_t<Target>,
            unqualified_t<source_value_type_t<SourceCapability>>> &&
        (alternative_type_count<
             source_value_type_t<SourceCapability>,
             unqualified_t<Target>>::value == 1)>> {
    template <typename Factory, typename Context, typename Capability>
    static Target& apply(Factory&, Context&, Capability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
        return resolve_materialized_alternative_reference<Target>(
            std::forward<Capability>(source),
            requested_type, registered_type);
    }
};

template <typename Target, typename SourceCapability>
struct type_conversion<
    Target, SourceCapability,
    std::enable_if_t<
        materialized_source_traits_t<SourceCapability>::reference_like &&
        is_alternative_type_v<source_value_type_t<SourceCapability>> &&
        std::is_same_v<
            unqualified_t<Target>,
            unqualified_t<source_value_type_t<SourceCapability>>>>> {
    template <typename Factory, typename Context, typename Capability>
    static Target& apply(Factory&, Context&, Capability&& source,
                         type_descriptor, type_descriptor) {
        return materialized_reference(source);
    }
};

template <typename Target, typename SourceCapability>
struct type_conversion<
    Target, SourceCapability,
    std::enable_if_t<
        materialized_source_traits_t<SourceCapability>::reference_like &&
        is_alternative_type_v<source_value_type_t<SourceCapability>> &&
        !std::is_pointer_v<Target> &&
        !std::is_same_v<
            unqualified_t<Target>,
            unqualified_t<source_value_type_t<SourceCapability>>> &&
        (alternative_type_count<
             source_value_type_t<SourceCapability>,
             unqualified_t<Target>>::value != 1)>> {
    template <typename Factory, typename Context, typename Capability>
    static Target& apply(Factory&, Context&, Capability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
        return resolve_alternative_borrowed_reference<Target>(
            materialized_reference(source), requested_type,
            registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, lvalue_source<Source>,
    std::enable_if_t<!type_traits<Source>::enabled && !std::is_pointer_v<Target> &&
                     !is_alternative_type_v<Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target& apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return materialized_reference(source);
    }
};

template <typename Target, typename SourceCapability>
struct type_conversion<
    Target*, SourceCapability,
    std::enable_if_t<
        materialized_source_traits_t<SourceCapability>::reference_like &&
        is_alternative_type_v<source_value_type_t<SourceCapability>> &&
        !std::is_same_v<
            unqualified_t<Target>,
            unqualified_t<source_value_type_t<SourceCapability>>> &&
        (alternative_type_count<
             source_value_type_t<SourceCapability>,
             unqualified_t<Target>>::value == 1)>> {
    template <typename Factory, typename Context, typename Capability>
    static Target* apply(Factory&, Context&, Capability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
        return resolve_materialized_alternative_pointer<Target>(
            std::forward<Capability>(source),
            requested_type, registered_type);
    }
};

template <typename Target, typename SourceCapability>
struct type_conversion<
    Target*, SourceCapability,
    std::enable_if_t<
        materialized_source_traits_t<SourceCapability>::reference_like &&
        is_alternative_type_v<source_value_type_t<SourceCapability>> &&
        std::is_same_v<
            unqualified_t<Target>,
            unqualified_t<source_value_type_t<SourceCapability>>>>> {
    template <typename Factory, typename Context, typename Capability>
    static Target* apply(Factory&, Context&, Capability&& source,
                         type_descriptor, type_descriptor) {
        return std::addressof(
            materialized_reference(source));
    }
};

template <typename Target, typename SourceCapability>
struct type_conversion<
    Target*, SourceCapability,
    std::enable_if_t<
        materialized_source_traits_t<SourceCapability>::reference_like &&
        is_alternative_type_v<source_value_type_t<SourceCapability>> &&
        !std::is_same_v<
            unqualified_t<Target>,
            unqualified_t<source_value_type_t<SourceCapability>>> &&
        (alternative_type_count<
             source_value_type_t<SourceCapability>,
             unqualified_t<Target>>::value != 1)>> {
    template <typename Factory, typename Context, typename Capability>
    static Target* apply(Factory&, Context&, Capability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
        return resolve_alternative_borrowed_pointer<Target>(
            materialized_reference(source), requested_type,
            registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, lvalue_source<Source>,
    std::enable_if_t<
        std::is_lvalue_reference_v<Target> &&
        is_pointer_like_type_v<
            std::remove_cv_t<std::remove_reference_t<Target>>> &&
        std::is_same_v<
            std::remove_cv_t<std::remove_reference_t<Target>>,
            Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target apply(Factory&, Context&, SourceCapability&& source,
                        type_descriptor, type_descriptor) {
        return materialized_reference(source);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, lvalue_source<Source>,
    std::enable_if_t<
        std::is_lvalue_reference_v<Target> &&
        is_pointer_like_type_v<
            std::remove_cv_t<std::remove_reference_t<Target>>> &&
        is_pointer_like_type_v<Source> &&
        !std::is_same_v<
            std::remove_cv_t<std::remove_reference_t<Target>>,
            Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target apply(Factory& factory, Context& context,
                        SourceCapability&& source,
                        type_descriptor requested_type,
                        type_descriptor registered_type) {
        using target_handle =
            std::remove_cv_t<std::remove_reference_t<Target>>;
        return resolve_handle_or_borrow<target_handle, Source>(
            factory, context, std::forward<SourceCapability>(source),
            requested_type, registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, lvalue_source<Source>,
    std::enable_if_t<type_traits<Target>::enabled &&
                     type_traits<Source>::enabled &&
                     !is_pointer_like_type_v<Target> &&
                     is_same_handle_shape_v<Target, Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target& apply(Factory&, Context&, SourceCapability&& source,
                         [[maybe_unused]] type_descriptor requested_type,
                         [[maybe_unused]] type_descriptor registered_type) {
        return resolve_materialized_convertible_reference<Target>(
            std::forward<SourceCapability>(source), requested_type,
            registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, lvalue_source<Source>,
    std::enable_if_t<type_traits<Target>::enabled &&
                     type_traits<Source>::enabled &&
                     !is_pointer_like_type_v<Target> &&
                     is_same_handle_shape_v<Target, Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&, SourceCapability&& source,
                         [[maybe_unused]] type_descriptor requested_type,
                         [[maybe_unused]] type_descriptor registered_type) {
        return resolve_materialized_convertible_pointer<Target>(
            std::forward<SourceCapability>(source), requested_type,
            registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, lvalue_source<Source>,
    std::enable_if_t<!std::is_pointer_v<Target> &&
                     is_borrowed_alternative_type_v<Source, Target>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target& apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return resolve_borrowed_materialized_value(
            std::forward<SourceCapability>(source));
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, lvalue_source<Source>,
    std::enable_if_t<
        !std::is_pointer_v<Target> &&
        is_borrowed_alternative_type_alternative_v<Source, Target>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target& apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
        return resolve_borrowed_materialized_alternative_reference<
            Target>(std::forward<SourceCapability>(source), requested_type,
                    registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, lvalue_source<Source>,
    std::enable_if_t<!type_traits<Target>::enabled && !std::is_pointer_v<Target> &&
                     type_traits<Source>::enabled &&
                     type_traits<Source>::is_value_borrowable &&
                     !is_borrowed_alternative_type_v<Source, Target> &&
                     !is_borrowed_alternative_type_alternative_v<Source,
                                                                         Target>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target& apply(Factory&, Context&,
                         SourceCapability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
        return resolve_borrowed_materialized_reference<Target>(
            std::forward<SourceCapability>(source), requested_type,
            registered_type);
    }
};

template <typename Target, size_t N, typename Source>
struct type_conversion<
    Target[N], pointer_source<Source>,
    std::enable_if_t<std::is_same_v<std::remove_cv_t<Source>,
                                    std::remove_cv_t<Target>>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target (&apply(Factory&, Context&, SourceCapability&& source,
                          type_descriptor, type_descriptor))[N] {
        return *reinterpret_cast<Target(*)[N]>(
            materialized_pointer(source));
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, lvalue_source<Source>,
    std::enable_if_t<is_borrowed_alternative_type_v<Source, Target>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return resolve_borrowed_materialized_value_pointer<Target>(
            std::forward<SourceCapability>(source));
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, lvalue_source<Source>,
    std::enable_if_t<
        is_borrowed_alternative_type_alternative_v<Source, Target>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
        return resolve_borrowed_materialized_alternative_pointer<Target>(
            std::forward<SourceCapability>(source), requested_type,
            registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, lvalue_source<Source>,
    std::enable_if_t<!type_traits<Target>::enabled &&
                     type_traits<Source>::enabled &&
                     type_traits<Source>::is_value_borrowable &&
                     !is_borrowed_alternative_type_v<Source, Target> &&
                     !is_borrowed_alternative_type_alternative_v<Source,
                                                                         Target>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&,
                         SourceCapability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
        return resolve_borrowed_materialized_pointer<Target>(
            std::forward<SourceCapability>(source), requested_type,
            registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, pointer_source<Source>,
    std::enable_if_t<std::is_array_v<Target> &&
                     std::is_same_v<std::remove_cv_t<Source>,
                                    std::remove_cv_t<Target>>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return materialized_pointer(source);
    }
};

template <typename Target, size_t N, typename Source>
struct type_conversion<
    Target (*)[N], pointer_source<Source>,
    std::enable_if_t<std::is_same_v<std::remove_cv_t<Source>,
                                    std::remove_cv_t<Target>>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target (*apply(Factory&, Context&, SourceCapability&& source,
                          type_descriptor, type_descriptor))[N] {
        return reinterpret_cast<Target(*)[N]>(
            materialized_pointer(source));
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, lvalue_source<Source>,
    std::enable_if_t<!type_traits<Target>::enabled &&
                     type_traits<Source>::enabled &&
                     !type_traits<Source>::is_value_borrowable>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&, SourceCapability&& source,
                         [[maybe_unused]] type_descriptor requested_type,
                         [[maybe_unused]] type_descriptor registered_type) {
        return resolve_materialized_get_pointer<Target, Source>(
            std::forward<SourceCapability>(source), requested_type,
            registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, lvalue_source<Source>,
    std::enable_if_t<is_pointer_like_type_v<Target> &&
                     std::is_same_v<Target, Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target& apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return materialized_reference(source);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, lvalue_source<Source>,
    std::enable_if_t<is_pointer_like_type_v<Target> &&
                     std::is_same_v<Target, Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return std::addressof(materialized_reference(source));
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, lvalue_source<Source>,
    std::enable_if_t<is_pointer_like_type_v<Target> &&
                     is_pointer_like_type_v<Source> &&
                     !std::is_same_v<Target, Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target& apply(Factory& factory, Context& context,
                         SourceCapability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
        return resolve_handle_or_borrow<Target, Source>(
            factory, context, std::forward<SourceCapability>(source),
            requested_type, registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, lvalue_source<Source>,
    std::enable_if_t<is_pointer_like_type_v<Target> &&
                     is_pointer_like_type_v<Source> &&
                     !std::is_same_v<Target, Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory& factory, Context& context,
                         SourceCapability&& source,
                         type_descriptor requested_type,
                         type_descriptor registered_type) {
        return resolve_handle_or_borrow_pointer<Target, Source>(
            factory, context, std::forward<SourceCapability>(source),
            requested_type, registered_type);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, pointer_source<Source>,
    std::enable_if_t<!std::is_array_v<Target> &&
                     !is_alternative_type_v<Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return materialized_pointer(source);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, pointer_source<Source>,
    std::enable_if_t<std::is_lvalue_reference_v<Target> &&
                     std::is_array_v<std::remove_reference_t<Target>>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target apply(Factory&, Context&, SourceCapability&& source,
                        type_descriptor, type_descriptor) {
        using array_type = std::remove_reference_t<Target>;
        return *reinterpret_cast<array_type*>(source.get());
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target, pointer_source<Source>,
    std::enable_if_t<!is_pointer_like_type_v<Target> &&
                     !std::is_pointer_v<Target> &&
                     !std::is_array_v<std::remove_reference_t<Target>> &&
                     !is_alternative_type_v<Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target& apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return materialized_reference(source);
    }
};

template <typename Target, typename Source>
struct type_conversion<
    Target*, lvalue_source<Source>,
    std::enable_if_t<!type_traits<Source>::enabled &&
                     !is_pointer_like_type_v<Target> &&
                     !is_alternative_type_v<Source>>> {
    template <typename Factory, typename Context, typename SourceCapability>
    static Target* apply(Factory&, Context&, SourceCapability&& source,
                         type_descriptor, type_descriptor) {
        return materialized_pointer(source);
    }
};
} // export namespace silicon::di

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// --- resolution/recursion_guard.h ---


export namespace silicon::di {

template <typename T,
          bool DefaultConstructible = std::is_default_constructible_v<T>>
struct recursion_guard {
    template <typename Context>
    explicit recursion_guard(Context& context)
        : frame_guard_(context.template track_type<T>()) {
        // Track the active type path first so recursion errors can report
        // the full resolution chain, including the repeated type.
        if (visited_) {
            ec_ = make_type_recursion_exception<T>(context);
            ok_ = false;
            active_ = false;
            return;
        }
        visited_ = true;
        ok_ = true;
    }

    recursion_guard(const recursion_guard&) = delete;
    recursion_guard& operator=(const recursion_guard&) = delete;
    recursion_guard(recursion_guard&&) = delete;
    recursion_guard& operator=(recursion_guard&&) = delete;

    ~recursion_guard() {
        if (active_) {
            visited_ = false;
        }
    }

    /// 解析是否成功进入（未被递归检测拦截）。调用方应检查并传播 error()。
    bool ok() const { return ok_; }
    std::error_code error() const { return ec_; }

  private:
    resolving_frame frame_guard_;
    bool active_ = true;
    bool ok_ = false;
    std::error_code ec_{};
    static thread_local bool visited_;
};

template <typename T, bool DefaultConstructible>
thread_local bool recursion_guard<T, DefaultConstructible>::visited_ = false;

template <typename T> struct recursion_guard<T, true> {
    template <typename Context>
    explicit recursion_guard(Context&) {}

    recursion_guard(const recursion_guard&) = delete;
    recursion_guard& operator=(const recursion_guard&) = delete;
    recursion_guard(recursion_guard&&) = delete;
    recursion_guard& operator=(recursion_guard&&) = delete;

    bool ok() const { return true; }
    std::error_code error() const { return {}; }
};

template <typename T> class recursion_guard_wrapper {
  public:
    template <typename Context>
    recursion_guard_wrapper(Context& context, bool enabled) {
        if (enabled) {
            // The wrapped recursion_guard owns a self-linking resolving_frame,
            // so it must stay at a fixed address for the whole guarded scope.
            new (&storage_) recursion_guard<T>(context);
            active_ = true;
        }
    }

    recursion_guard_wrapper(const recursion_guard_wrapper&) = delete;
    recursion_guard_wrapper& operator=(const recursion_guard_wrapper&) = delete;
    recursion_guard_wrapper(recursion_guard_wrapper&&) = delete;
    recursion_guard_wrapper& operator=(recursion_guard_wrapper&&) = delete;

    ~recursion_guard_wrapper() {
        if (active_) {
            get()->~recursion_guard<T>();
        }
    }

  private:
    recursion_guard<T>* get() {
        return reinterpret_cast<recursion_guard<T>*>(&storage_);
    }

    aligned_storage_t<sizeof(recursion_guard<T>), alignof(recursion_guard<T>)>
        storage_;
    bool active_ = false;
};

} // export namespace silicon::di


// ==============================================================================
// ==  storage  —  storage policies (shared / unique / external / cyclical)
// ==============================================================================

// --- storage/type_storage_traits.h ---




export namespace silicon::di {


struct no_materialization_scope {
    no_materialization_scope() = default;

    template <typename... Args>
    explicit no_materialization_scope(Args&&...) {}
};


template <typename StorageTag, typename Type> struct storage_materialization_traits {
    template <typename Leaf, typename Context, typename Storage>
    static auto make_guard(Context&, const Storage&) {
        static_assert(always_false_v<StorageTag, Type>,
                      "storage_materialization_traits must be specialized for this storage tag");
    }

    template <typename Storage>
    static bool preserves_closure(const Storage&) {
        static_assert(always_false_v<StorageTag, Type>,
                      "storage_materialization_traits must be specialized for this storage tag");
        return false;
    }

    template <typename Context, typename Storage, typename Container>
    static auto materialize_source(Context&, Storage&, Container&) {
        static_assert(always_false_v<StorageTag, Type>,
                      "storage_materialization_traits must be specialized for this storage tag");
    }
};

template <typename StorageTag, typename Type, typename U, typename = void>
struct resolution_traits {
    using value_types = type_list<>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<>;
    using conversion_types = type_list<>;
};


template <typename AccessTraits, typename ResolutionTraits>
struct combined_storage_types {
    using value_types = type_list_cat_t<typename AccessTraits::value_types,
                                        typename ResolutionTraits::value_types>;
    using lvalue_reference_types =
        type_list_cat_t<typename AccessTraits::lvalue_reference_types,
                        typename ResolutionTraits::lvalue_reference_types>;
    using rvalue_reference_types =
        type_list_cat_t<typename AccessTraits::rvalue_reference_types,
                        typename ResolutionTraits::rvalue_reference_types>;
    using pointer_types =
        type_list_cat_t<typename AccessTraits::pointer_types,
                        typename ResolutionTraits::pointer_types>;
    using conversion_types =
        type_list_cat_t<typename AccessTraits::conversion_types,
                        typename ResolutionTraits::conversion_types>;
};

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <typename T, typename List> struct has_distinct_conversion_type;

template <typename T>
struct has_distinct_conversion_type<T, type_list<>> : std::false_type {};

template <typename T, typename Head, typename... Tail>
struct has_distinct_conversion_type<T, type_list<Head, Tail...>>
    : std::bool_constant<
          !std::is_same_v<T, remove_cvref_t<Head>> ||
          has_distinct_conversion_type<T, type_list<Tail...>>::value> {};

template <typename T, typename List>
inline constexpr bool has_distinct_conversion_type_v =
    has_distinct_conversion_type<T, List>::value;

template <typename T, typename List> struct max_distinct_conversion_size;

template <typename T>
struct max_distinct_conversion_size<T, type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename T, typename Head, typename... Tail>
struct max_distinct_conversion_size<T, type_list<Head, Tail...>>
    : std::integral_constant<
          std::size_t,
          !std::is_same_v<T, remove_cvref_t<Head>>
              ? (sizeof(remove_cvref_t<Head>) >
                         max_distinct_conversion_size<T,
                                                      type_list<Tail...>>::value
                     ? sizeof(remove_cvref_t<Head>)
                     : max_distinct_conversion_size<T,
                                                    type_list<Tail...>>::value)
              : max_distinct_conversion_size<T, type_list<Tail...>>::value> {};

template <typename T, typename List>
inline constexpr std::size_t max_distinct_conversion_size_v =
    max_distinct_conversion_size<T, List>::value;

template <typename T, typename List> struct max_distinct_conversion_align;

template <typename T>
struct max_distinct_conversion_align<T, type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename T, typename Head, typename... Tail>
struct max_distinct_conversion_align<T, type_list<Head, Tail...>>
    : std::integral_constant<
          std::size_t,
          !std::is_same_v<T, remove_cvref_t<Head>>
              ? (alignof(remove_cvref_t<Head>) >
                         max_distinct_conversion_align<
                             T, type_list<Tail...>>::value
                     ? alignof(remove_cvref_t<Head>)
                     : max_distinct_conversion_align<T,
                                                     type_list<Tail...>>::value)
              : max_distinct_conversion_align<T, type_list<Tail...>>::value> {};

template <typename T, typename List>
inline constexpr std::size_t max_distinct_conversion_align_v =
    max_distinct_conversion_align<T, List>::value;

template <typename T, typename List>
struct has_nontrivial_distinct_conversion_destructor;

template <typename T>
struct has_nontrivial_distinct_conversion_destructor<T, type_list<>>
    : std::false_type {};

template <typename T, typename Head, typename... Tail>
struct has_nontrivial_distinct_conversion_destructor<T,
                                                     type_list<Head, Tail...>>
    : std::bool_constant<
          (!std::is_same_v<T, remove_cvref_t<Head>> &&
           !std::is_trivially_destructible_v<remove_cvref_t<Head>>) ||
          has_nontrivial_distinct_conversion_destructor<
              T, type_list<Tail...>>::value> {};

template <typename T, typename List>
inline constexpr bool has_nontrivial_distinct_conversion_destructor_v =
    has_nontrivial_distinct_conversion_destructor<T, List>::value;

template <typename Storage, typename = void>
struct static_conversion_temporary_slots_base
    : std::integral_constant<std::size_t, 0> {};

template <typename Storage>
struct static_conversion_temporary_slots_base<
    Storage,
    std::void_t<
        decltype(Storage::static_conversion_temporary_slots)>>
    : std::integral_constant<
          std::size_t,
          Storage::static_conversion_temporary_slots> {};

template <typename Storage, typename = void>
struct static_conversion_temporary_size_base
    : std::integral_constant<std::size_t, 0> {};

template <typename Storage>
struct static_conversion_temporary_size_base<
    Storage,
    std::void_t<
        decltype(Storage::static_conversion_temporary_size)>>
    : std::integral_constant<
          std::size_t,
          Storage::static_conversion_temporary_size> {};

template <typename Storage, typename = void>
struct static_conversion_temporary_align_base
    : std::integral_constant<std::size_t, 0> {};

template <typename Storage>
struct static_conversion_temporary_align_base<
    Storage,
    std::void_t<
        decltype(Storage::static_conversion_temporary_align)>>
    : std::integral_constant<
          std::size_t,
          Storage::static_conversion_temporary_align> {};

template <typename Storage, typename = void>
struct static_conversion_destructible_slots_base
    : std::integral_constant<std::size_t, 0> {};

template <typename Storage>
struct static_conversion_destructible_slots_base<
    Storage,
    std::void_t<
        decltype(Storage::static_conversion_destructible_slots)>>
    : std::integral_constant<
          std::size_t,
          Storage::static_conversion_destructible_slots> {};

template <typename Storage, typename = void>
struct static_conversion_temporary_slots
    : static_conversion_temporary_slots_base<Storage> {};

template <typename Storage>
struct static_conversion_temporary_slots<
    Storage, std::void_t<typename Storage::conversions>>
    : static_conversion_temporary_slots_base<typename Storage::conversions> {};

template <typename Storage, typename = void>
struct static_conversion_temporary_size
    : static_conversion_temporary_size_base<Storage> {};

template <typename Storage>
struct static_conversion_temporary_size<
    Storage, std::void_t<typename Storage::conversions>>
    : static_conversion_temporary_size_base<typename Storage::conversions> {};

template <typename Storage, typename = void>
struct static_conversion_temporary_align
    : static_conversion_temporary_align_base<Storage> {};

template <typename Storage>
struct static_conversion_temporary_align<
    Storage, std::void_t<typename Storage::conversions>>
    : static_conversion_temporary_align_base<typename Storage::conversions> {};

template <typename Storage, typename = void>
struct static_conversion_destructible_slots
    : static_conversion_destructible_slots_base<Storage> {};

template <typename Storage>
struct static_conversion_destructible_slots<
    Storage, std::void_t<typename Storage::conversions>>
    : static_conversion_destructible_slots_base<typename Storage::conversions> {
};


template <typename Storage>
inline constexpr std::size_t static_conversion_temporary_slots_v =
    static_conversion_temporary_slots<Storage>::value;

template <typename Storage>
inline constexpr std::size_t static_conversion_temporary_size_v =
    static_conversion_temporary_size<Storage>::value;

template <typename Storage>
inline constexpr std::size_t static_conversion_temporary_align_v =
    static_conversion_temporary_align<Storage>::value;

template <typename Storage>
inline constexpr std::size_t static_conversion_destructible_slots_v =
    static_conversion_destructible_slots<Storage>::value;

template <typename StorageTag, typename Type, typename U, typename = void>
struct type_storage_traits;

template <typename StorageTag, typename Type, typename U>
struct type_storage_traits<
    StorageTag, Type, U,
    std::enable_if_t<storage_traits<StorageTag, Type, U>::enabled>>
    : combined_storage_types<storage_traits<StorageTag, Type, U>,
                                     resolution_traits<StorageTag, Type, U>> {
  private:
    using combined_types =
        combined_storage_types<storage_traits<StorageTag, Type, U>,
                                       resolution_traits<StorageTag, Type, U>>;

  public:
    static constexpr bool is_stable =
        storage_traits<StorageTag, Type, U>::is_stable;
    static constexpr std::size_t static_conversion_temporary_slots =
        has_distinct_conversion_type_v<
            Type, typename combined_types::conversion_types> &&
                !is_stable
            ? 1
            : 0;
    static constexpr std::size_t static_conversion_temporary_size =
        !is_stable ? max_distinct_conversion_size_v<
                         Type, typename combined_types::conversion_types>
                   : 0;
    static constexpr std::size_t static_conversion_temporary_align =
        !is_stable ? max_distinct_conversion_align_v<
                         Type, typename combined_types::conversion_types>
                   : 0;
    static constexpr std::size_t static_conversion_destructible_slots =
        !is_stable && has_nontrivial_distinct_conversion_destructor_v<
                          Type, typename combined_types::conversion_types>
            ? 1
            : 0;
};
} // export namespace silicon::di


// ==============================================================================
// ==  type  —  type traits, descriptors, type lists & maps
// ==============================================================================

// --- type/request_traits.h ---



export namespace silicon::di {

template <typename Request>
using request_interface_t = typename annotated_traits<Request>::type;

template <typename Request>
using request_value_t = normalized_type_t<Request>;

template <typename Request>
using request_result_t =
    request_interface_t<std::conditional_t<std::is_rvalue_reference_v<Request>,
                                           std::remove_reference_t<Request>,
                                           Request>>;

template <typename Request, bool RemoveRvalueReferences>
using resolve_request_t =
    std::conditional_t<RemoveRvalueReferences,
                       std::conditional_t<std::is_rvalue_reference_v<Request>,
                                          std::remove_reference_t<Request>,
                                          Request>,
                       Request>;

template <typename Request, bool RemoveRvalueReferences>
using resolve_result_t =
    request_interface_t<resolve_request_t<Request, RemoveRvalueReferences>>;


template <typename Request>
using resolve_expected_t = as_expected_t<request_interface_t<Request>>;

template <typename Request, bool RemoveRvalueReferences>
using resolve_expected_result_t =
    as_expected_t<resolve_result_t<Request, RemoveRvalueReferences>>;

} // export namespace silicon::di


// ==============================================================================
// ==  core  —  binding model, binding collection & context
// ==============================================================================

// --- core/binding_resolution.h ---




#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

export namespace silicon::di {


template <typename Storage, typename T, typename Context, typename Source,
          typename = void>
struct has_storage_resolve_conversion : std::false_type {};

template <typename Storage, typename T, typename Context, typename Source>
struct has_storage_resolve_conversion<
    Storage, T, Context, Source,
    std::void_t<
        decltype(std::declval<Storage&>().template resolve_conversion<T>(
            std::declval<Context&>(), std::declval<Source>()))>>
    : std::true_type {};

template <bool Enabled, typename ConversionTypes>
struct binding_conversion_cache_base;

template <typename ConversionTypes>
struct binding_conversion_cache_base<true, ConversionTypes> {
    template <typename T, typename Context, typename... Args>
    T& construct_conversion(Context&, Args&&... args) {
        return conversions_.template construct<T>(std::forward<Args>(args)...);
    }

  protected:
    conversion_cache<ConversionTypes> conversions_;
};

template <typename ConversionTypes>
struct binding_conversion_cache_base<false, ConversionTypes> {
    template <typename T, typename Context, typename... Args>
    T& construct_conversion(Context& context, Args&&... args) {
        return context.template construct<T>(std::forward<Args>(args)...);
    }
};

template <typename Context> struct preserve_closure_scope {
    explicit preserve_closure_scope(Context& context)
        : context_(context), exceptions_(std::uncaught_exceptions()) {}

    ~preserve_closure_scope() {
        if (std::uncaught_exceptions() == exceptions_) {
            context_.pop();
        }
    }

  private:
    Context& context_;
    int exceptions_;
};

enum class binding_request_kind {
    kValue,
    kLvalueReference,
    kRvalueReference,
    kPointer,
};

template <typename RTTI> struct binding_request {
    instance_request<RTTI> request;
    instance_cache_sink cache;
    binding_request_kind kind;
};

template <typename T, typename RTTI>
constexpr binding_request<RTTI>
make_binding_request(instance_cache_sink cache = {}) {
    return {
        {RTTI::template get_type_index<request_lookup_type_t<T>>(),
         describe_type<T>()},
        cache,
        std::is_pointer_v<T>            ? binding_request_kind::kPointer
        : std::is_lvalue_reference_v<T> ? binding_request_kind::kLvalueReference
        : std::is_rvalue_reference_v<T> ? binding_request_kind::kRvalueReference
                                        : binding_request_kind::kValue};
}

template <typename T> T convert_resolved_binding(void* ptr) {
    using result_type = std::remove_reference_t<T>;

    if constexpr (std::is_lvalue_reference_v<T>) {
        return *static_cast<result_type*>(ptr);
    } else if constexpr (std::is_rvalue_reference_v<T>) {
        return std::move(*static_cast<result_type*>(ptr));
    } else if constexpr (std::is_pointer_v<T>) {
        return static_cast<T>(ptr);
    } else if constexpr (is_copy_constructible_v<T>) {
        return *static_cast<T*>(ptr);
    } else {
        return std::move(*static_cast<T*>(ptr));
    }
}

template <typename T, typename Instance, typename Fn>
decltype(auto) forward_resolved_binding(Instance&& instance, Fn&& fn) {
    if constexpr (std::is_lvalue_reference_v<T>) {
        return std::forward<Fn>(fn)(instance);
    } else if constexpr (std::is_rvalue_reference_v<T>) {
        return std::forward<Fn>(fn)(std::move(instance));
    } else if constexpr (std::is_pointer_v<T>) {
        return std::forward<Fn>(fn)(instance);
    } else {
        return std::forward<Fn>(fn)(std::forward<Instance>(instance));
    }
}

template <typename T, typename Instance, typename Fn>
decltype(auto) consume_resolved_binding(Instance&& instance, Fn&& fn) {
    if constexpr (std::is_lvalue_reference_v<T>) {
        return std::forward<Fn>(fn)(instance);
    } else if constexpr (std::is_rvalue_reference_v<T>) {
        return std::forward<Fn>(fn)(std::move(instance));
    } else if constexpr (std::is_pointer_v<T>) {
        return std::forward<Fn>(fn)(instance);
    } else if constexpr (is_copy_constructible_v<T>) {
        using value_type = std::remove_reference_t<T>;
        return std::forward<Fn>(fn)(
            value_type(std::forward<Instance>(instance)));
    } else {
        return std::forward<Fn>(fn)(std::move(instance));
    }
}

template <typename ExactLookup>
constexpr bool matches_exact_lookup(type_descriptor requested_type) {
    if (requested_type == describe_type<ExactLookup>()) {
        return true;
    }

    if constexpr (std::is_lvalue_reference_v<ExactLookup>) {
        using target_type = std::remove_reference_t<ExactLookup>;
        return requested_type == describe_type<const target_type&>();
    } else if constexpr (std::is_pointer_v<ExactLookup>) {
        using target_type = std::remove_pointer_t<ExactLookup>;
        return requested_type == describe_type<const target_type*>();
    } else {
        return false;
    }
}

template <typename Target, typename Context, typename T>
void* get_address_as(Context& context, T&& instance) {
    using instance_type = std::remove_reference_t<T>;

    if constexpr (std::is_pointer_v<instance_type>) {
        return const_cast<std::remove_const_t<std::remove_pointer_t<
            instance_type>>*>(instance);
    } else if constexpr (std::is_reference_v<T>) {
        return const_cast<std::remove_const_t<instance_type>*>(&instance);
    } else {
        return &context.template construct<Target>(std::forward<T>(instance));
    }
}

template <typename Storage, typename Context, typename Owner, typename Fn>
decltype(auto) materialize_binding_source(Context& context, Storage& storage,
                                          Owner& owner, Fn&& fn) {
    using materialization_traits =
        storage_materialization_traits<typename Storage::tag_type,
                                       typename Storage::type>;

    auto source =
        materialization_traits::materialize_source(context, storage, owner);
    return std::forward<Fn>(fn)(std::move(source));
}

template <typename Storage, typename Context, typename Owner, typename Fn>
decltype(auto) materialize_tracked_binding_source(Context& context,
                                                  Storage& storage,
                                                  Owner& owner, Fn&& fn) {
    using materialization_traits =
        storage_materialization_traits<typename Storage::tag_type,
                                       typename Storage::type>;
    using leaf_type = leaf_type_t<typename Storage::type>;

    [[maybe_unused]] auto guard =
        materialization_traits::template make_guard<leaf_type>(context,
                                                               storage);
    auto source =
        materialization_traits::materialize_source(context, storage, owner);
    return std::forward<Fn>(fn)(std::move(source));
}

template <typename Storage, typename Context, typename Owner, typename Closure,
          typename Fn>
decltype(auto)
materialize_binding_resolution_source(Context& context, Storage& storage,
                                      Owner& owner, Closure& closure, Fn&& fn) {
    using materialization_traits =
        storage_materialization_traits<typename Storage::tag_type,
                                       typename Storage::type>;
    using leaf_type = leaf_type_t<typename Storage::type>;

    [[maybe_unused]] auto guard =
        materialization_traits::template make_guard<leaf_type>(context,
                                                               storage);
    if (materialization_traits::preserves_closure(storage)) {
        context.push(&closure);
        preserve_closure_scope<Context> closure_scope(context);
        auto source =
            materialization_traits::materialize_source(context, storage, owner);
        return std::forward<Fn>(fn)(std::move(source));
    }

    auto source =
        materialization_traits::materialize_source(context, storage, owner);
    return std::forward<Fn>(fn)(std::move(source));
}

template <typename T, typename Storage, typename ConversionResolver,
          typename Context, typename Source>
decltype(auto) resolve_binding_conversion(Storage& storage,
                                          ConversionResolver& resolver,
                                          Context& context, Source&& source) {
    if constexpr (has_storage_resolve_conversion<Storage, T, Context,
                                                 Source&&>::value) {
        return storage.template resolve_conversion<T>(
            context, std::forward<Source>(source));
    } else {
        return resolver.template construct_conversion<T>(
            context, std::forward<Source>(source));
    }
}

template <typename Target, typename Resolver, typename Context,
          typename SourceCapability>
decltype(auto) resolve_binding_value(Resolver& resolver, Context& context,
                                     SourceCapability&& source) {
    using source_capability = std::remove_reference_t<SourceCapability>;
    using source_type = decltype(std::declval<source_capability>().get());

    if constexpr (std::is_same_v<Target, source_type>) {
        return std::forward<SourceCapability>(source).get();
    } else {
        return resolver.template resolve_conversion<Target>(
            context, std::forward<SourceCapability>(source).get());
    }
}

template <typename Request, typename Storage, typename Resolver,
          typename Context, typename SourceCapability>
decltype(auto) resolve_binding_request(Resolver& resolver, Context& context,
                                       SourceCapability&& source) {
    using source_capability = std::remove_reference_t<SourceCapability>;
    using source_type = decltype(std::declval<source_capability>().get());

    if constexpr (std::is_same_v<Request, source_type>) {
        return std::forward<SourceCapability>(source).get();
    } else {
        return type_conversion<Request, source_capability>::apply(
            resolver, context, std::forward<SourceCapability>(source),
            describe_type<Request>(), describe_type<typename Storage::type>());
    }
}

template <typename Request, typename Storage, typename Resolver,
          typename Context, typename SourceCapability, typename Fn>
decltype(auto)
handle_resolved_binding_request(Resolver& resolver, Context& context,
                                SourceCapability&& source, Fn&& fn) {
    auto&& instance = resolve_binding_request<Request, Storage>(
        resolver, context, std::forward<SourceCapability>(source));
    return forward_resolved_binding<Request>(
        std::forward<decltype(instance)>(instance), std::forward<Fn>(fn));
}

template <typename Request, typename Storage, typename Resolver,
          typename Context, typename SourceCapability, typename Fn>
decltype(auto)
consume_resolved_binding_request(Resolver& resolver, Context& context,
                                 SourceCapability&& source, Fn&& fn) {
    auto&& instance = resolve_binding_request<Request, Storage>(
        resolver, context, std::forward<SourceCapability>(source));
    return consume_resolved_binding<Request>(
        std::forward<decltype(instance)>(instance), std::forward<Fn>(fn));
}

template <typename Request, typename Storage, typename Resolver,
          typename Context, typename Owner, typename Fn>
decltype(auto) forward_binding_request(Context& context, Storage& storage,
                                       Owner& owner, Resolver& resolver,
                                       Fn&& fn) {
    return materialize_tracked_binding_source(
        context, storage, owner, [&](auto&& source) -> decltype(auto) {
            return handle_resolved_binding_request<Request, Storage>(
                resolver, context, std::forward<decltype(source)>(source),
                std::forward<Fn>(fn));
        });
}

template <typename Request, typename Storage, typename Resolver,
          typename Context, typename Owner, typename Closure, typename Fn>
decltype(auto)
forward_binding_resolution_request(Context& context, Storage& storage,
                                   Owner& owner, Closure& closure,
                                   Resolver& resolver, Fn&& fn) {
    return materialize_binding_resolution_source(
        context, storage, owner, closure, [&](auto&& source) -> decltype(auto) {
            return handle_resolved_binding_request<Request, Storage>(
                resolver, context, std::forward<decltype(source)>(source),
                std::forward<Fn>(fn));
        });
}

template <typename Request, typename Storage, typename Resolver,
          typename Context, typename Owner, typename Fn>
decltype(auto) consume_binding_request(Context& context, Storage& storage,
                                       Owner& owner, Resolver& resolver,
                                       Fn&& fn) {
    return materialize_tracked_binding_source(
        context, storage, owner, [&](auto&& source) -> decltype(auto) {
            return consume_resolved_binding_request<Request, Storage>(
                resolver, context, std::forward<decltype(source)>(source),
                std::forward<Fn>(fn));
        });
}

template <typename Request, typename Storage, typename Resolver,
          typename Context, typename Owner, typename Closure, typename Fn>
decltype(auto)
consume_binding_resolution_request(Context& context, Storage& storage,
                                   Owner& owner, Closure& closure,
                                   Resolver& resolver, Fn&& fn) {
    return materialize_binding_resolution_source(
        context, storage, owner, closure, [&](auto&& source) -> decltype(auto) {
            return consume_resolved_binding_request<Request, Storage>(
                resolver, context, std::forward<decltype(source)>(source),
                std::forward<Fn>(fn));
        });
}

template <typename Target, typename Resolver, typename Context,
          typename SourceCapability>
decltype(auto) apply_binding_conversion_from_source(
    Resolver& resolver, Context& context, SourceCapability&& source,
    type_descriptor requested_type, type_descriptor registered_type) {
    using source_capability = std::remove_reference_t<SourceCapability>;
    return type_conversion<Target, source_capability>::apply(
        resolver, context, std::forward<SourceCapability>(source),
        requested_type, registered_type);
}

template <typename Target, typename Context, typename Instance>
void* resolve_binding_address_from_instance(Context& context,
                                            Instance&& instance) {
    return get_address_as<Target>(
        context, std::forward<decltype(instance)>(instance));
}

template <typename Target, typename Resolver, typename Context,
          typename SourceCapability>
void* resolve_binding_address_from_source(Resolver& resolver, Context& context,
                                          SourceCapability&& source,
                                          type_descriptor requested_type,
                                          type_descriptor registered_type) {
    return resolve_binding_address_from_instance<Target>(
        context, apply_binding_conversion_from_source<Target>(
                     resolver, context, std::forward<SourceCapability>(source),
                     requested_type, registered_type));
}

template <typename Target, typename Request, typename Registered,
          typename Resolver, typename Context, typename SourceCapability>
void* resolve_static_binding_address_from_source(Resolver& resolver,
                                                 Context& context,
                                                 SourceCapability&& source) {
    return resolve_binding_address_from_instance<Target>(
        context, apply_binding_conversion_from_source<Target>(
                     resolver, context, std::forward<SourceCapability>(source),
                     describe_type<Request>(), describe_type<Registered>()));
}

template <typename RTTI, typename Binding, typename Context>
void* dispatch_binding_request(Binding& binding, Context& context,
                               const binding_request<RTTI>& request) {
    switch (request.kind) {
    case binding_request_kind::kPointer:
        return binding.get_pointer(context, request.request, request.cache);
    case binding_request_kind::kLvalueReference:
        return binding.get_lvalue_reference(context, request.request,
                                            request.cache);
    case binding_request_kind::kRvalueReference:
        return binding.get_rvalue_reference(context, request.request,
                                            request.cache);
    case binding_request_kind::kValue:
        return binding.get_value(context, request.request, request.cache);
    }

    std::unreachable();
}



template <typename T, typename RTTI, typename Binding, typename Context>
T resolve_binding_request(Binding& binding, Context& context,
                          instance_cache_sink cache = {}) {
    auto request = make_binding_request<T, RTTI>(cache);
    void* ptr =
        dispatch_binding_request<RTTI>(binding, context, request);
    return convert_resolved_binding<T>(ptr);
}

template <typename RTTI, typename Factory, typename Context, typename... Types>
as_expected_t<void*> resolve_binding_capability_address(Factory& factory, Context& context,
                                         type_list<Types...>,
                                         const typename RTTI::type_index& type,
                                         type_descriptor requested_type,
                                         type_descriptor registered_type) {
    void* address = nullptr;
    const bool matched =
        ((RTTI::template get_type_index<lookup_type_t<Types>>() == type
              ? (address = factory.template resolve_address<Types>(
                     context, requested_type, registered_type),
                 true)
              : false) ||
         ...);

    if (!matched) {
        return std::unexpected(make_type_not_convertible_exception(
            requested_type, registered_type, context));
    }

    return address;
}

template <typename Request>
inline constexpr bool can_wrap_normalized_request_v =
    type_traits<std::decay_t<Request>>::enabled &&
    !std::is_pointer_v<std::decay_t<Request>> &&
    !std::is_same_v<normalized_type_t<Request>, std::decay_t<Request>>;

// Rvalue requests are not "construct me a normalized value" requests. They
// are asking the routed storage to publish a movable result, so they must
// reject normalized/wrapper construction instead of silently degrading to `T`.
template <typename Request>
inline constexpr bool rvalue_request_requires_explicit_conversion_v =
    std::is_rvalue_reference_v<Request>;

template <typename Request>
inline constexpr bool construct_normalized_request_v =
    can_wrap_normalized_request_v<Request> &&
    !rvalue_request_requires_explicit_conversion_v<Request> &&
    std::is_object_v<normalized_type_t<Request>> &&
    !std::is_abstract_v<normalized_type_t<Request>>;

template <typename Request, typename ResolvedRequest = request_result_t<Request>>
inline constexpr bool construct_factory_request_v =
    !rvalue_request_requires_explicit_conversion_v<Request> &&
    std::is_object_v<normalized_type_t<Request>> &&
    !std::is_abstract_v<normalized_type_t<Request>> &&
    (std::is_pointer_v<ResolvedRequest> ||
     (type_traits<ResolvedRequest>::enabled &&
      !std::is_reference_v<ResolvedRequest>) ||
     construction_traits<ResolvedRequest,
                         normalized_type_t<Request>>::enabled ||
     std::is_constructible_v<ResolvedRequest, normalized_type_t<Request>>);

template <typename Request, typename MakeNotConvertible, typename MakeNotFound>
[[noreturn]] void
terminate_missing_rvalue_conversion(bool has_normalized_request,
                                MakeNotConvertible&& make_not_convertible,
                                MakeNotFound&& make_not_found) {
    static_assert(rvalue_request_requires_explicit_conversion_v<Request>);

    // di 已统一错误体系：make_* 现在返回 std::error_code。
    // 此 [[noreturn]] 包装器用于 rvalue 显式转换缺失的硬失败路径，
    // 调用方已声明 as_expected_t<...> 返回类型；为避免在调用点连锁改写，
    // 这里直接用 std::terminate 落地，并通过 std::unexpected 取出 error_code
    // 的诊断信息输出到 stderr。
    std::error_code ec = has_normalized_request
                             ? std::forward<MakeNotConvertible>(
                                   make_not_convertible)()
                             : std::forward<MakeNotFound>(make_not_found)();
    std::cerr << "silicon::di: rvalue conversion failed: " << ec.message()
              << "\n";
    std::terminate();
}

template <typename Request, typename Context>
[[noreturn]] void terminate_missing_rvalue_conversion(bool has_normalized_request,
                                                  Context& context) {
    using normalized_request_type = normalized_type_t<Request>;
    terminate_missing_rvalue_conversion<Request>(
        has_normalized_request,
        [&]() {
            return make_type_not_convertible_exception(
                describe_type<Request>(),
                describe_type<normalized_request_type>(), context);
        },
        [&]() {
            return make_type_not_found_exception<Request>(context);
        });
}

template <typename Request>
[[noreturn]] void terminate_missing_rvalue_conversion(bool has_normalized_request) {
    using normalized_request_type = normalized_type_t<Request>;
    terminate_missing_rvalue_conversion<Request>(
        has_normalized_request,
        [&]() {
            return make_type_not_convertible_exception(
                describe_type<Request>(),
                describe_type<normalized_request_type>());
        },
        [&]() { return make_type_not_found_exception<Request>(); });
}

template <typename Request, typename ResolveExact, typename ResolveNormalized>
request_result_t<Request>
construct_request_or_wrap_normalized(ResolveExact&& resolve_exact,
                                     ResolveNormalized&& resolve_normalized) {
    // expected 语义：仅当精确解析以 kTypeNotConvertible 失败且支持
    // normalized 降级时，改用 normalized 类型解析并构造；其余失败原样返回。
    auto exact = std::forward<ResolveExact>(resolve_exact)();
    if (!exact && can_wrap_normalized_request_v<Request> &&
        exact.error() == make_error_code(di_error::kTypeNotConvertible)) {
        auto value = std::forward<ResolveNormalized>(resolve_normalized)();
        return type_traits<std::decay_t<Request>>::make(
            std::forward<decltype(value)>(value));
    }
    return exact;
}
} // export namespace silicon::di

#ifdef _MSC_VER
#pragma warning(pop)
#endif


// ==============================================================================
// ==  runtime  —  runtime container traits & registry
// ==============================================================================

// --- runtime/context.h ---


export namespace silicon::di {

class runtime_context : public context_state {
  public:
    template <typename T, typename Container>
    T resolve(Container& container) {
        if constexpr (is_keyed_v<T>) {
            using request_type = keyed_type_t<T>;
            using key_type = keyed_key_t<T>;
            return T(container.template resolve<request_type, false, true>(
                *this, key<key_type>{}));
        } else {
            return container.template resolve<T, false>(*this);
        }
    }

    template <typename T, typename DetectionTag, typename Container>
    T construct_temporary(Container& container) {
        using temporary_type = normalized_type_t<T>;

        auto& active_closure = *closures_.back();
        arena_allocator<void> alloc(active_closure.arena_storage());
        auto allocator = allocator_traits::rebind<temporary_type>(alloc);
        auto instance = allocator_traits::allocate(allocator, 1);
        default_constructor_detection<temporary_type, DetectionTag>()
            .template construct<temporary_type>(instance, *this, container);
        if constexpr (!std::is_trivially_destructible_v<temporary_type>) {
            register_destructor(instance);
        }

        if constexpr (std::is_lvalue_reference_v<T>) {
            return *instance;
        } else {
            return std::move(*instance);
        }
    }
};

} // export namespace silicon::di


// ==============================================================================
// ==  resolution  —  type resolution, conversion cache & recursion guards
// ==============================================================================

// --- resolution/runtime_binding.h ---




export namespace silicon::di {
// TODO: this is bit convoluted, ideally merge resolver with runtime binding

template <typename InstanceContainer, typename Storage,
          typename ResolutionContainer = InstanceContainer>
class runtime_binding_state {
  public:
    using container_type = InstanceContainer;
    using instance_container_type = InstanceContainer;
    using resolution_container_type = ResolutionContainer;

    template <typename ParentContainer, typename... Args>
    runtime_binding_state(ParentContainer* parent, Args&&... args)
        : storage_(std::forward<Args>(args)...),
          instance_container_(parent, parent->get_allocator()),
          resolution_container_(parent, parent->get_allocator()) {}

    Storage& storage_ref() { return storage_; }
    InstanceContainer& instance_container_ref() { return instance_container_; }
    ResolutionContainer& resolution_container_ref() {
        return resolution_container_;
    }

  private:
    Storage storage_;
    InstanceContainer instance_container_;
    ResolutionContainer resolution_container_;
};

template <typename InstanceContainer, typename Storage>
class runtime_binding_state<InstanceContainer, Storage, InstanceContainer> {
  public:
    using container_type = InstanceContainer;
    using instance_container_type = InstanceContainer;
    using resolution_container_type = InstanceContainer;

    template <typename ParentContainer, typename... Args>
    runtime_binding_state(ParentContainer* parent, Args&&... args)
        : storage_(std::forward<Args>(args)...),
          instance_container_(parent, parent->get_allocator()) {}

    Storage& storage_ref() { return storage_; }
    InstanceContainer& instance_container_ref() { return instance_container_; }
    InstanceContainer& resolution_container_ref() {
        return instance_container_;
    }

  private:
    Storage storage_;
    InstanceContainer instance_container_;
};

template <typename T> struct runtime_binding_state_traits {
    using state_type = T;

    static T& ref(T& state) { return state; }
};

template <typename T> struct runtime_binding_state_traits<std::shared_ptr<T>> {
    using state_type = T;

    static T& ref(std::shared_ptr<T>& state) { return *state; }
};


template <typename Storage> using registered_type_t = typename Storage::type;

template <typename Type, typename Storage>
using runtime_binding_conversion_types_t =
    rebind_leaf_t<typename Storage::conversions::conversion_types, Type>;

template <typename Types>
inline constexpr bool runtime_binding_has_conversion_cache_v =
    type_list_size_v<Types> != 0;



// TODO: the container here is just for RTTI, but it is needed to get the
// inner container type and that is very hard. Perhaps pass RTTI and inner
// container directly?
template <typename Container, typename Type, typename Storage, typename State>
class runtime_binding
    : public runtime_binding_interface<Container>,
      private binding_conversion_cache_base<
          Storage::conversions::is_stable &&
              runtime_binding_has_conversion_cache_v<
                  runtime_binding_conversion_types_t<Type, Storage>>,
          runtime_binding_conversion_types_t<Type, Storage>> {
  public:
    using storage_type = Storage;
    using state_traits = runtime_binding_state_traits<State>;
    using state_type = typename state_traits::state_type;
    using container_type = typename state_type::container_type;
    using rtti_type = typename Container::rtti_type;
    using type_index = typename rtti_type::type_index;
    using request_type = instance_request<rtti_type>;
    using conversion_types =
        runtime_binding_conversion_types_t<Type, Storage>;
    using exact_value_types = std::conditional_t<
        type_traits<Type>::enabled && !std::is_pointer_v<Type>,
        type_list<Type>, type_list<>>;
    using exact_lvalue_reference_types = std::conditional_t<
        Storage::conversions::is_stable && type_traits<Type>::enabled &&
            !std::is_pointer_v<Type>,
        type_list<Type&>, type_list<>>;
    using exact_pointer_types = std::conditional_t<
        Storage::conversions::is_stable && type_traits<Type>::enabled &&
            !std::is_pointer_v<Type>,
        type_list<Type*>, type_list<>>;
    using value_capability_types = type_list_cat_t<
        exact_value_types, typename Storage::conversions::value_types>;
    using lvalue_reference_capability_types = type_list_cat_t<
        exact_lvalue_reference_types,
        typename Storage::conversions::lvalue_reference_types>;
    using pointer_capability_types = type_list_cat_t<
        exact_pointer_types, typename Storage::conversions::pointer_types>;
    static constexpr bool has_conversion_cache =
        runtime_binding_has_conversion_cache_v<conversion_types>;
    static constexpr bool uses_cached_conversions =
        Storage::conversions::is_stable && has_conversion_cache;
    using materialization_traits =
        storage_materialization_traits<typename Storage::tag_type,
                                       typename Storage::type>;
    using conversion_cache_base =
        binding_conversion_cache_base<uses_cached_conversions,
                                              conversion_types>;

  private:
    using conversion_cache_base::construct_conversion;

    struct binding_activation {
        runtime_binding& binding;

        template <typename T, typename Context, typename Source>
        decltype(auto) construct_conversion(Context& context, Source&& source) {
            return binding.template construct_conversion<T>(
                context, std::forward<Source>(source));
        }

        template <typename T, typename Context, typename Source>
        decltype(auto) resolve_conversion(Context& context, Source&& source) {
            return binding.template resolve_conversion<T>(
                context, std::forward<Source>(source));
        }
    };

    context_closure closure_;
    // `state_` must be destroyed before binding state so shared storage can
    // tear down cached instances before preserved construction temporaries.
    State state_;

    state_type& state_ref() { return state_traits::ref(state_); }
    auto& get_storage() { return state_ref().storage_ref(); }
    auto& get_resolution_container() {
        return state_ref().resolution_container_ref();
    }

    static constexpr type_descriptor registered_type() {
        return describe_type<registered_type_t<Storage>>();
    }

  public:
    template <typename T, typename Context, typename Source>
    decltype(auto) resolve_conversion(Context& context, Source&& source) {
        binding_activation activation{*this};
        return resolve_binding_conversion<T>(
            get_storage(), activation, context, std::forward<Source>(source));
    }

    template <typename ConversionTypes>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
    void* convert(runtime_context& context, const request_type& request,
                  instance_cache_sink cache) {
        void* ptr = ::silicon::di::resolve_binding_capability_address<rtti_type>(
            *this, context, ConversionTypes{}, request.lookup_type,
            request.requested_type, registered_type());
        // Request caching is intentionally stricter than conversion caching.
        // shared_cyclical shared_ptr storage, for example, can keep converted
        // handles alive in the storage while still deferring publication of a
        // request result until the outer resolve has committed successfully.
        if constexpr (Storage::conversions::is_stable) {
            cache(ptr);
        }
        return ptr;
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

  public:
    template <typename... Args>
    runtime_binding(Args&&... args) : state_(std::forward<Args>(args)...) {}

    auto& get_container() { return state_ref().instance_container_ref(); }

    void* get_value(runtime_context& context, const request_type& request,
                    instance_cache_sink cache) override {
        return convert<value_capability_types>(context, request, cache);
    }

    void* get_lvalue_reference(runtime_context& context,
                               const request_type& request,
                               instance_cache_sink cache) override {
        return convert<lvalue_reference_capability_types>(
            context, request, cache);
    }

    void* get_rvalue_reference(runtime_context& context,
                               const request_type& request,
                               instance_cache_sink cache) override {
        return convert<typename Storage::conversions::rvalue_reference_types>(
            context, request, cache);
    }

    void* get_pointer(runtime_context& context, const request_type& request,
                      instance_cache_sink cache) override {
        return convert<pointer_capability_types>(context, request, cache);
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
    template <typename T, typename Context>
    as_expected_t<void*> resolve_address(Context& context, type_descriptor requested_type,
                          type_descriptor registered_type) {
        if constexpr (is_exact_lookup_v<T>) {
            if (!matches_exact_lookup<resolved_type_t<T, Type>>(
                    requested_type)) {
                return std::unexpected(make_type_not_convertible_exception(
                    requested_type, registered_type, context));
            }
        }

        using Target = std::remove_reference_t<resolved_type_t<T, Type>>;
        return materialize_binding_resolution_source(
            context, get_storage(), get_resolution_container(), closure_,
            [&](auto&& source) -> void* {
                return resolve_binding_address_from_source<Target>(
                    *this, context, std::forward<decltype(source)>(source),
                    requested_type, registered_type);
            });
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    template <typename Context> decltype(auto) resolve(Context& context) {
        return materialize_binding_resolution_source(
            context, get_storage(), get_resolution_container(), closure_,
            [](auto&& source) -> decltype(auto) {
                return std::forward<decltype(source)>(source).get();
            });
    }

    template <typename T, typename Context>
    decltype(auto) resolve(Context& context) {
        binding_activation activation{*this};
        return materialize_binding_resolution_source(
            context, get_storage(), get_resolution_container(), closure_,
            [&](auto&& source) -> decltype(auto) {
                return resolve_binding_value<T>(
                    activation, context,
                    std::forward<decltype(source)>(source));
            });
    }

    void destroy() override {
        auto allocator = allocator_traits::rebind<runtime_binding>(
            get_container().get_allocator());
        allocator_traits::destroy(allocator, this);
        allocator_traits::deallocate(allocator, this, 1);
    }
};

// This is much faster than unique_ptr + deleter
template <typename T> struct runtime_binding_ptr {
    using destroy_fn = void (*)(T*);

    runtime_binding_ptr(T* ptr = nullptr, destroy_fn destroy = &default_destroy)
        : ptr_(ptr), destroy_(destroy) {}

    runtime_binding_ptr(const runtime_binding_ptr&) = delete;
    runtime_binding_ptr& operator=(const runtime_binding_ptr&) = delete;

    runtime_binding_ptr(runtime_binding_ptr<T>&& other) noexcept
        : ptr_(other.release()), destroy_(other.destroy_) {
        other.destroy_ = &default_destroy;
    }

    runtime_binding_ptr& operator=(runtime_binding_ptr<T>&& other) noexcept {
        if (this != &other) {
            reset();
            ptr_ = other.release();
            destroy_ = other.destroy_;
            other.destroy_ = &default_destroy;
        }
        return *this;
    }

    ~runtime_binding_ptr() { reset(); }

    T& operator*() {
        assert(ptr_);
        return *ptr_;
    }

    T* release() {
        T* ptr = ptr_;
        ptr_ = nullptr;
        return ptr;
    }

    void reset(T* ptr = nullptr, destroy_fn destroy = &default_destroy) {
        if (ptr_) {
            destroy_(ptr_);
        }
        ptr_ = ptr;
        destroy_ = destroy;
    }

    T* get() { return ptr_; }
    T* operator->() { return ptr_; }
    operator bool() const { return ptr_ != nullptr; }

  private:
    static void default_destroy(T* ptr) { ptr->destroy(); }

    T* ptr_ = nullptr;
    destroy_fn destroy_ = &default_destroy;
};

} // export namespace silicon::di

// --- resolution/type_cache.h ---


// #include <unordered_map>

export namespace silicon::di {
template <typename Value, typename RTTI, typename Allocator>
struct dynamic_type_cache {
    dynamic_type_cache(Allocator& allocator) : values_(allocator) {}

    template <typename Key, typename ValueT>
    void insert(ValueT&& value) {
        auto pb = values_.emplace(RTTI::template get_type_index<Key>(),
                                  std::forward<ValueT>(value));
        (void)pb;
        assert(pb.second);
    }

    template <typename Key> const Value& get() {
        auto it = values_.find(RTTI::template get_type_index<Key>());
        if (it != values_.end()) return it->second;
        return empty_;
    }

  private:
    using allocator_type =
        typename std::allocator_traits<Allocator>::template rebind_alloc<
            std::pair<const typename RTTI::type_index, Value>>;

    std::map<typename RTTI::type_index, Value,
             std::less<typename RTTI::type_index>, allocator_type>
        values_;
    // std::unordered_map< typename RTTI::type_index, Value, typename
    // RTTI::type_index::hasher, std::equal_to< typename RTTI::type_index >,
    // map_allocator_type > values_;

    static Value empty_;
};

template <typename Value, typename RTTI, typename Allocator>
Value dynamic_type_cache<Value, RTTI, Allocator>::empty_;

template <typename Value, typename Tag> struct static_type_cache_node {
    Value value;
    static_type_cache_node<Value, Tag>* next = nullptr;
#if !defined(NDEBUG)
    const void* owner;
#endif
};

template <typename Key, typename Value, typename Tag>
struct static_type_cache_node_factory {
    static static_type_cache_node<Value, Tag> node;
};

template <typename Key, typename Value, typename Tag>
static_type_cache_node<Value, Tag>
    static_type_cache_node_factory<Key, Value, Tag>::node;

template <typename Value, typename Tag,
          typename Allocator = std::allocator<void>>
struct static_type_cache {
    template <typename Key>
    using node_factory = static_type_cache_node_factory<Key, Value, Tag>;
    using node_type = static_type_cache_node<Value, Tag>;

    static_type_cache() = default;
    static_type_cache(Allocator&) {}

    ~static_type_cache() {
        auto node = nodes_;
        while (node) {
            assert(node->value ? node->owner == this : node->owner == nullptr);
            node->value = Value();
#if !defined(NDEBUG)
            node->owner = nullptr;
#endif
            node = node->next;
        }
    }

    template <typename Key, typename ValueT> void insert(ValueT&& value) {
        auto& node = node_factory<Key>::node;
        assert(!node.value);
        assert(node.owner == nullptr);
        node.value = std::forward<ValueT>(value);
        node.next = nodes_;
        nodes_ = &node;
        ++size_;
#if !defined(NDEBUG)
        node.owner = this;
#endif
    }

    template <typename Key> const Value& get() {
        auto& node = node_factory<Key>::node;
        return node.value;
    }

  private:
    node_type* nodes_ = nullptr;
    size_t size_ = 0;
};
} // export namespace silicon::di


// ==============================================================================
// ==  rtti  —  runtime type info & typeid providers
// ==============================================================================

// --- rtti/rtti.h ---


export namespace silicon::di {
    struct static_provider {};
    struct typeid_provider {};

    template< typename T > class rtti;
}

// --- rtti/static_provider.h ---



export namespace silicon::di {

template<> class rtti<static_provider> {
    template <typename T> struct type_index_tag {
        // TODO: This will not work across modules.
        // Should probably detect it first, handle it next.
        static constexpr size_t tag{};
    };

  public:
    class type_index {
        friend struct std::hash<type_index>;
      public:
        constexpr type_index(size_t value) : value_(value) {}

        constexpr bool operator<(const type_index& other) const {
            return value_ < other.value_;
        }

        constexpr bool operator==(const type_index& other) const {
            return value_ == other.value_;
        }

      private:
        size_t value_;
    };

    template <typename T> static constexpr type_index get_type_index() {
        return reinterpret_cast<size_t>(&type_index_tag<T>::tag);
    }
};
} // export namespace silicon::di

namespace std {
    template<> struct hash<typename silicon::di::rtti<silicon::di::static_provider>::type_index> {
        size_t operator()(const typename silicon::di::rtti<silicon::di::static_provider>::type_index& value) const {
            return hash<size_t>()(value.value_);
        }
    };
}

// --- rtti/typeid_provider.h ---



export namespace silicon::di {

template<> class rtti<typeid_provider> {
    template <typename T> struct wrapper {};

  public:
    class type_index {
        friend struct std::hash<type_index>;
      public:
        type_index(std::type_index value) : value_(value) {}

        bool operator<(const type_index& other) const {
            return value_ < other.value_;
        }
        bool operator==(const type_index& other) const {
            return value_ == other.value_;
        }

      private:       
        std::type_index value_;
    };

    template <typename T> static type_index get_type_index() {
        return std::type_index(typeid(wrapper<T>));
    }
};

} // export namespace silicon::di

namespace std {
    template<> struct hash<typename silicon::di::rtti<silicon::di::typeid_provider>::type_index> {
        size_t operator()(const typename silicon::di::rtti<silicon::di::typeid_provider>::type_index& value) const {
            return hash<std::type_index>()(value.value_);
        }
    };
}


// ==============================================================================
// ==  type  —  type traits, descriptors, type lists & maps
// ==============================================================================

// --- type/type_map.h ---


// #include <unordered_map>

export namespace silicon::di {
template <typename Value, typename RTTI, typename Allocator>
struct dynamic_type_map {
    dynamic_type_map(Allocator& allocator) : values_(allocator) {}

    template <typename Key, typename... Args>
    std::pair<Value&, bool> insert(Args&&... args) {
        auto pb = values_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(RTTI::template get_type_index<Key>()),
            std::forward_as_tuple(std::forward<Args>(args)...));
        return {pb.first->second, pb.second};
    }

    template <typename Key> bool erase() {
        return values_.erase(RTTI::template get_type_index<Key>());
    }

    template <typename Key> Value* get() {
        auto it = values_.find(RTTI::template get_type_index<Key>());
        return it != values_.end() ? &it->second : nullptr;
    }

    size_t size() const { return values_.size(); }

    Value& front() {
        assert(!values_.empty());
        return values_.begin()->second;
    }

    auto begin() { return values_.begin(); }
    auto end() { return values_.end(); }

  private:
    using allocator_type =
        typename std::allocator_traits<Allocator>::template rebind_alloc<
            std::pair<const typename RTTI::type_index, Value>>;

    std::map<typename RTTI::type_index, Value,
             std::less<typename RTTI::type_index>, allocator_type>
        values_;
    // std::unordered_map< typename RTTI::type_index, Value, typename
    //    std::hash< typename RTTI::type_index>, std::equal_to< typename RTTI::type_index >,
    //    allocator_type > values2_;
};

template <typename Value, typename Tag> struct static_type_map_node {
    std::optional<Value> value;
    static_type_map_node<Value, Tag>* next = nullptr;
#if !defined(NDEBUG)
    const void* owner;
#endif
};

template <typename Key, typename Value, typename Tag>
struct static_type_map_node_factory {
    static static_type_map_node<Value, Tag> node;
};

template <typename Key, typename Value, typename Tag>
static_type_map_node<Value, Tag>
    static_type_map_node_factory<Key, Value, Tag>::node;

template <typename Value, typename Tag,
          typename Allocator = std::allocator<void>>
struct static_type_map {
    template <typename Key>
    using node_factory = static_type_map_node_factory<Key, Value, Tag>;
    using node_type = static_type_map_node<Value, Tag>;

    static_type_map() : nodes_(), size_() {}
    static_type_map(Allocator&) : nodes_(), size_() {}

    ~static_type_map() {
        auto node = nodes_;
        while (node) {
            assert(node->value ? node->owner == this : node->owner == nullptr);
            node->value.reset();
#if !defined(NDEBUG)
            node->owner = nullptr;
#endif
            node = node->next;
        }
    }

    template <typename Key, typename... Args>
    std::pair<Value&, bool> insert(Args&&... args) {
        auto& node = node_factory<Key>::node;
        if (!node.value) {
            assert(node.owner == nullptr);
            node.value.emplace(std::forward<Args>(args)...);
            node.next = nodes_;
            nodes_ = &node;
            ++size_;
#if !defined(NDEBUG)
            node.owner = this;
#endif
            return {*node.value, true};
        }
        assert(node.owner ==);
        return {*node.value, false};
    }

    template <typename Key> bool erase() {
        auto& node = node_factory<Key>::node;
        if (!node.value)
            return false;
        assert(node.owner == this);

        auto current = &nodes_;
        while (*current && *current != &node) {
            current = &(*current)->next;
        }
        assert(*current == &node);
        *current = node.next;

        node.value.reset();
        node.next = nullptr;
        --size_;
#if !defined(NDEBUG)
        node.owner = nullptr;
#endif
        return true;
    }

    template <typename Key> Value* get() {
        auto& node = node_factory<Key>::node;
        assert(node.value ? node.owner == this : node.owner == nullptr);
        return node.value ? &*node.value : nullptr;
    }

    size_t size() const { return size_; }

    Value& front() {
        assert(nodes_);
        return *nodes_->value;
    }

    struct iterator {
        iterator(node_type* node) : node_(node) {}

        iterator& operator++() {
            assert(node_);
            node_ = node_->next;
            return *this;
        }

        iterator operator++(int) {
            assert(node_);
            auto n = node_;
            node_ = node_->next;
            return n;
        }

        bool operator==(iterator other) const { return node_ == other.node_; }
        bool operator!=(iterator other) const { return node_ != other.node_; }

        std::pair<void*, Value&> operator*() {
            assert(node_);
            return {nullptr, *node_->value};
        }

      private:
        node_type* node_;
    };

    iterator begin() { return nodes_; }

    iterator end() { return nullptr; }

  private:
    node_type* nodes_;
    size_t size_;
};
} // export namespace silicon::di


// ==============================================================================
// ==  runtime  —  runtime container traits & registry
// ==============================================================================

// --- runtime/container_traits.h ---



export namespace silicon::di {

struct dynamic_container_traits {
    template <typename> using rebind_t = dynamic_container_traits;

    using tag_type = none_t;
    using rtti_type = rtti<typeid_provider>;
    template <typename Value, typename Allocator>
    using type_map_type = dynamic_type_map<Value, rtti_type, Allocator>;
    template <typename Value, typename Allocator>
    using type_cache_type = dynamic_type_cache<Value, rtti_type, Allocator>;
    using allocator_type = std::allocator<char>;
    using index_definition_type = std::tuple<>;
    static constexpr bool cache_enabled = true;
};



template <typename T, typename = void>
struct is_runtime_container_traits : std::false_type {};

template <typename T>
struct is_runtime_container_traits<
    T, std::void_t<typename T::tag_type, typename T::rtti_type,
                   typename T::allocator_type,
                   typename T::index_definition_type,
                   typename T::template rebind_t<void>>>
    : std::true_type {};

template <typename T>
inline constexpr bool is_runtime_container_traits_v =
    is_runtime_container_traits<T>::value;

template <typename Traits>
inline constexpr bool is_tagged_container_v =
    !std::is_same_v<typename Traits::template rebind_t<
                        type_list<typename Traits::tag_type, void>>,
                    Traits>;


} // export namespace silicon::di

// --- runtime/registration_api.h ---



export namespace silicon::di {


template <typename Derived> class runtime_registration_api {
    Derived& self() { return static_cast<Derived&>(*this); }

  public:
    decltype(auto) get_allocator() {
        return self().runtime_registry_ref().get_allocator();
    }

    template <typename... TypeArgs> auto& register_type() {
        return self().runtime_registry_ref().template emplace_type_binding<
            TypeArgs...>(self().runtime_registration_parent(), none_t{},
                         none_t{});
    }

    template <typename... TypeArgs, typename Arg>
    auto& register_type(Arg&& arg) {
        return self().runtime_registry_ref().template emplace_type_binding<
            TypeArgs...>(self().runtime_registration_parent(),
                         std::forward<Arg>(arg), none_t{});
    }

    template <typename... TypeArgs, typename IdType>
    auto& register_indexed_type(IdType&& id) {
        return self().runtime_registry_ref().template emplace_type_binding<
            TypeArgs...>(self().runtime_registration_parent(), none_t{},
                         std::forward<IdType>(id));
    }

    template <typename... TypeArgs, typename Arg, typename IdType>
    auto& register_indexed_type(Arg&& arg, IdType&& id) {
        return self().runtime_registry_ref().template emplace_type_binding<
            TypeArgs...>(self().runtime_registration_parent(),
                         std::forward<Arg>(arg), std::forward<IdType>(id));
    }

    template <typename... TypeArgs, typename Fn>
    auto& register_type_collection(Fn&& fn) {
        using registration = type_registration<TypeArgs...>;
        return register_type<TypeArgs...>(callable(
            [this, collection_fn = std::forward<Fn>(fn)]() mutable {
                return self().template construct_collection<
                    typename registration::storage_type::type>(collection_fn);
            }));
    }

    template <typename... TypeArgs> auto& register_type_collection() {
        return register_type_collection<TypeArgs...>(
            binding_collection_append{});
    }
};


} // export namespace silicon::di


// ==============================================================================
// ==  static  —  static container graph, registry & resolution
// ==============================================================================

// --- static/registry.h ---



export namespace silicon::di {

template <typename... Args> using bind = type_registration<Args...>;



enum class dependency_resolution_status {
    kResolved,
    kMissing,
    kAmbiguous,
};

template <typename BindingModel>
using binding_dependencies_t = typename BindingModel::dependencies_type::type;

template <typename...> inline constexpr bool dependent_false_v = false;

template <typename Head, typename List> struct type_list_prepend;

template <typename Head, typename... Tail>
struct type_list_prepend<Head, type_list<Tail...>> {
    using type = type_list<Head, Tail...>;
};

template <typename Head, typename Tuples> struct prepend_binding_to_tuples;

template <typename Head, typename... Tuples>
struct prepend_binding_to_tuples<Head, type_list<Tuples...>> {
    using type = type_list<typename type_list_prepend<Head, Tuples>::type...>;
};

template <typename Interface, typename RequestTypes>
struct annotated_request_types {
    using type = RequestTypes;
};

template <typename T, typename Tag, typename... RequestTypes>
struct annotated_request_types<annotated<T, Tag>, type_list<RequestTypes...>> {
    using type = type_list<annotated<RequestTypes, Tag>...>;
};

template <typename Key, typename RequestTypes> struct keyed_request_types {
    using type = RequestTypes;
};

template <typename Key, typename... RequestTypes>
struct keyed_request_types<Key, type_list<RequestTypes...>> {
    using type =
        std::conditional_t<std::is_void_v<Key>, type_list<RequestTypes...>,
                           type_list<keyed<RequestTypes, Key>...>>;
};

template <typename Request>
using binding_request_interface_t = normalized_type_t<keyed_type_t<Request>>;

template <typename Request>
using binding_exact_request_interface_t =
    std::remove_cv_t<std::remove_reference_t<keyed_type_t<Request>>>;

template <typename Request> using binding_request_key_t = keyed_key_t<Request>;

template <typename Binding> struct binding_request_types {
  private:
    using interface_type = typename Binding::interface_type;
    using binding_model_type = typename Binding::binding_model_type;
    using raw_interface_type = typename annotated_traits<interface_type>::type;
    using storage_conversions =
        typename binding_model_type::storage_type::conversions;
    using exact_interface_value_types = std::conditional_t<
        type_traits<raw_interface_type>::enabled &&
            !std::is_pointer_v<raw_interface_type>,
        type_list<raw_interface_type>, type_list<>>;
    using exact_interface_lvalue_reference_types = std::conditional_t<
        storage_conversions::is_stable &&
            type_traits<raw_interface_type>::enabled &&
            !std::is_pointer_v<raw_interface_type>,
        type_list<raw_interface_type&>, type_list<>>;
    using exact_interface_pointer_types = std::conditional_t<
        storage_conversions::is_stable &&
            type_traits<raw_interface_type>::enabled &&
            !std::is_pointer_v<raw_interface_type>,
        type_list<raw_interface_type*>, type_list<>>;
    using base_types = type_list_cat_t<
        exact_interface_value_types, exact_interface_lvalue_reference_types,
        exact_interface_pointer_types,
        rebind_leaf_t<typename storage_conversions::value_types,
                      raw_interface_type>,
        rebind_leaf_t<typename storage_conversions::lvalue_reference_types,
                      raw_interface_type>,
        rebind_leaf_t<typename storage_conversions::rvalue_reference_types,
                      raw_interface_type>,
        rebind_leaf_t<typename storage_conversions::pointer_types,
                      raw_interface_type>>;

  public:
    using type = type_list_unique_t<typename keyed_request_types<
        typename Binding::key_type,
        typename annotated_request_types<interface_type,
                                         base_types>::type>::type>;
};

template <typename Request> struct binding_request_converter {
    operator Request() const;
};

template <typename Implementation, typename BindingTuple>
struct binding_tuple_constructible;

template <typename Implementation, typename SelectedRequests,
          typename BindingTuple>
struct binding_tuple_request_constructible;

template <typename Implementation, typename SelectedRequests,
          typename RequestTypes, typename RemainingBindings>
struct binding_request_options_constructible;

template <typename Implementation, typename... SelectedRequests>
struct binding_tuple_request_constructible<Implementation,
                                           type_list<SelectedRequests...>,
                                           type_list<>>
    : std::bool_constant<
          is_list_initializable_v<
              Implementation,
              binding_request_converter<SelectedRequests>...> ||
          is_direct_initializable_v<
              Implementation,
              binding_request_converter<SelectedRequests>...>> {};

template <typename Implementation, typename... SelectedRequests, typename Head,
          typename... Tail>
struct binding_tuple_request_constructible<Implementation,
                                           type_list<SelectedRequests...>,
                                           type_list<Head, Tail...>>
    : binding_request_options_constructible<
          Implementation, type_list<SelectedRequests...>,
          typename binding_request_types<Head>::type, type_list<Tail...>> {};

template <typename Implementation, typename SelectedRequests,
          typename RemainingBindings>
struct binding_request_options_constructible<Implementation, SelectedRequests,
                                             type_list<>,
                                             RemainingBindings>
    : std::false_type {};

template <typename Implementation, typename... SelectedRequests,
          typename Request, typename... Requests, typename RemainingBindings>
struct binding_request_options_constructible<
    Implementation, type_list<SelectedRequests...>, type_list<Request, Requests...>,
    RemainingBindings>
    : std::bool_constant<
          binding_tuple_request_constructible<
              Implementation, type_list<SelectedRequests..., Request>,
              RemainingBindings>::value ||
          binding_request_options_constructible<
              Implementation, type_list<SelectedRequests...>,
              type_list<Requests...>, RemainingBindings>::value> {};

template <typename Implementation, typename... InterfaceBindings>
struct binding_tuple_constructible<Implementation,
                                   type_list<InterfaceBindings...>>
    : binding_tuple_request_constructible<Implementation, type_list<>,
                                          type_list<InterfaceBindings...>> {};

template <typename InterfaceBindings, size_t Arity, typename = void>
struct binding_tuples;

template <typename InterfaceBindings>
struct binding_tuples<InterfaceBindings, 0, void> {
    using type = type_list<type_list<>>;
};

template <typename... InterfaceBindings, size_t Arity>
struct binding_tuples<type_list<InterfaceBindings...>, Arity,
                      std::enable_if_t<(Arity > 0)>> {
  private:
    using tail_tuples =
        typename binding_tuples<type_list<InterfaceBindings...>,
                                Arity - 1>::type;

  public:
    using type = type_list_cat_t<typename prepend_binding_to_tuples<
        InterfaceBindings, tail_tuples>::type...>;
};

template <typename Implementation, typename CandidateTuples>
struct unique_constructible_binding_tuple;

template <typename Implementation>
struct unique_constructible_binding_tuple<Implementation, type_list<>> {
    static constexpr size_t count = 0;
    using type = void;
};

template <typename Implementation, typename Head, typename... Tail>
struct unique_constructible_binding_tuple<Implementation,
                                          type_list<Head, Tail...>> {
  private:
    using tail_result =
        unique_constructible_binding_tuple<Implementation, type_list<Tail...>>;
    static constexpr bool head_matches =
        binding_tuple_constructible<Implementation, Head>::value;

  public:
    static constexpr size_t count = tail_result::count + (head_matches ? 1 : 0);
    using type = std::conditional_t<
        count == 1,
        std::conditional_t<head_matches, Head, typename tail_result::type>,
        void>;
};

template <typename Factory> struct constructor_factory_target;

template <typename T> struct constructor_factory_target<constructor<T>> {
    using type = normalized_type_t<T>;
};

template <typename BindingModel, typename InterfaceBindings>
struct remove_current_binding_model;

template <typename BindingModel>
struct remove_current_binding_model<BindingModel, type_list<>> {
    using type = type_list<>;
};

template <typename BindingModel, typename Head, typename... Tail>
struct remove_current_binding_model<BindingModel, type_list<Head, Tail...>> {
  private:
    using tail_type =
        typename remove_current_binding_model<BindingModel,
                                              type_list<Tail...>>::type;

  public:
    using type = std::conditional_t<
        std::is_same_v<BindingModel, typename Head::binding_model_type>,
        tail_type, type_list_cat_t<type_list<Head>, tail_type>>;
};

template <typename BindingModel, typename InterfaceBindings, typename = void>
struct inferred_binding_dependencies {
    using type = void;
    static constexpr dependency_resolution_status status =
        dependency_resolution_status::kMissing;
};

template <typename BindingModel, typename InterfaceBindings>
struct inferred_binding_dependencies<
    BindingModel, InterfaceBindings,
    std::enable_if_t<is_plain_constructor_factory<
        typename BindingModel::factory_type>::value>> {
  private:
    using factory_type = typename BindingModel::factory_type;
    using implementation_type =
        typename constructor_factory_target<factory_type>::type;
    using candidate_bindings =
        typename remove_current_binding_model<BindingModel,
                                              InterfaceBindings>::type;
    using candidate_tuples =
        typename binding_tuples<candidate_bindings, factory_type::arity>::type;
    using selection = unique_constructible_binding_tuple<implementation_type,
                                                         candidate_tuples>;

  public:
    using type = std::conditional_t<factory_type::arity == 0, type_list<>,
                                    typename selection::type>;
    static constexpr dependency_resolution_status status =
        factory_type::arity == 0
            ? dependency_resolution_status::kResolved
            : (selection::count == 0 ? dependency_resolution_status::kMissing
               : selection::count == 1
                   ? dependency_resolution_status::kResolved
                   : dependency_resolution_status::kAmbiguous);
};

template <typename DependencyList, typename InterfaceBindings>
struct dependency_bindings;

template <typename ResolvedBindings> struct dependency_bindings_are_resolved;

template <typename BindingModel, typename InterfaceBindings,
          bool HasKnownDependencies =
              !std::is_same_v<binding_dependencies_t<BindingModel>, void>>
struct binding_dependency_resolution;

template <typename BindingModel, typename InterfaceBindings>
struct binding_dependency_resolution<BindingModel, InterfaceBindings, true> {
    using type =
        typename dependency_bindings<binding_dependencies_t<BindingModel>,
                                     InterfaceBindings>::type;
    static constexpr dependency_resolution_status status =
        dependency_bindings_are_resolved<type>::value
            ? dependency_resolution_status::kResolved
            : dependency_resolution_status::kMissing;
};

template <typename BindingModel, typename InterfaceBindings>
struct binding_dependency_resolution<BindingModel, InterfaceBindings, false> {
    using type =
        typename inferred_binding_dependencies<BindingModel,
                                               InterfaceBindings>::type;
    static constexpr dependency_resolution_status status =
        inferred_binding_dependencies<BindingModel, InterfaceBindings>::status;
};

template <typename Interface, typename Key, typename InterfaceBinding>
struct binding_matches
    : std::bool_constant<
          std::is_same_v<Interface,
                         typename InterfaceBinding::interface_type> &&
          (std::is_void_v<Key> ||
           std::is_same_v<Key, typename InterfaceBinding::key_type>)> {};

template <typename Interface, typename Key, typename InterfaceBindings>
struct binding_interface_match;

template <typename Interface, typename Key>
struct binding_interface_match<Interface, Key, type_list<>> {
    using type = type_list<>;
};

template <typename Interface, typename Key, typename Head, typename... Tail>
struct binding_interface_match<Interface, Key, type_list<Head, Tail...>> {
  private:
    using tail_type =
        typename binding_interface_match<Interface, Key, type_list<Tail...>>::type;

  public:
    using type =
        std::conditional_t<binding_matches<Interface, Key, Head>::value,
                           type_list_cat_t<type_list<Head>, tail_type>,
                           tail_type>;
};

template <typename Interface, typename Key, typename InterfaceBindings>
using bindings_t = typename binding_interface_match<Interface, Key, InterfaceBindings>::type;

template <typename Interface, typename Key, typename InterfaceBindings>
struct binding_count;

template <typename Interface, typename Key>
struct binding_count<Interface, Key, type_list<>>
    : std::integral_constant<size_t, 0> {};

template <typename Interface, typename Key, typename Head, typename... Tail>
struct binding_count<Interface, Key, type_list<Head, Tail...>>
    : std::integral_constant<
          size_t,
          (binding_matches<Interface, Key, Head>::value ? 1 : 0) +
              binding_count<Interface, Key, type_list<Tail...>>::value> {};

template <typename Interface, typename Key, typename InterfaceBindings>
inline constexpr size_t binding_count_v =
    binding_count<Interface, Key, InterfaceBindings>::value;

template <typename Bindings,
          bool HasSingleBinding = (type_list_size_v<Bindings> == 1)>
struct single_binding;

template <typename Bindings> struct single_binding<Bindings, false> {
    using type = void;
};

template <typename Head, typename... Tail>
struct single_binding<type_list<Head, Tail...>, true> {
    using type = Head;
};

template <typename Interface, typename Key, typename InterfaceBindings>
struct binding_lookup {
    using type = typename single_binding<
        bindings_t<Interface, Key, InterfaceBindings>>::type;
};

template <typename Interface, typename Key, typename InterfaceBindings>
using binding_t =
    typename binding_lookup<Interface, Key, InterfaceBindings>::type;

template <typename InterfaceBindings> struct keyed_bindings_are_unique;

template <> struct keyed_bindings_are_unique<type_list<>> : std::true_type {};

template <typename Head, typename... Tail>
struct keyed_bindings_are_unique<type_list<Head, Tail...>>
    : std::bool_constant<
          (std::is_void_v<typename Head::key_type> ||
           binding_count_v<typename Head::interface_type,
                           typename Head::key_type, type_list<Head, Tail...>> ==
               1) &&
          keyed_bindings_are_unique<type_list<Tail...>>::value> {};

template <typename DependencyList, typename InterfaceBindings>
struct dependencies_registered;

template <typename DependencyList, typename InterfaceBindings>
struct first_missing_declared_dependency;

template <typename InterfaceBindings>
struct first_missing_declared_dependency<void, InterfaceBindings> {
    using type = void;
};

template <typename Dependency, typename InterfaceBindings,
          bool IsCollection = collection_traits<
              binding_request_interface_t<Dependency>>::is_collection>
struct declared_dependency_is_registered;

template <typename Dependency, typename InterfaceBindings>
struct declared_dependency_is_registered<Dependency, InterfaceBindings, false> {
  private:
    using dependency_type = binding_request_interface_t<Dependency>;
    using dependency_key = binding_request_key_t<Dependency>;

  public:
    static constexpr bool value =
        !std::is_void_v<
            binding_t<dependency_type, dependency_key, InterfaceBindings>>;
};

template <typename Dependency, typename InterfaceBindings>
struct declared_dependency_is_registered<Dependency, InterfaceBindings, true> {
  private:
    using dependency_type = binding_request_interface_t<Dependency>;
    using dependency_key = binding_request_key_t<Dependency>;
    using collection_type = collection_traits<dependency_type>;

  public:
    static constexpr bool value =
        binding_count_v<normalized_type_t<typename collection_type::resolve_type>,
                        dependency_key, InterfaceBindings> != 0;
};

template <typename InterfaceBindings>
struct first_missing_declared_dependency<type_list<>, InterfaceBindings> {
    using type = void;
};

template <typename Head, typename... Tail, typename InterfaceBindings>
struct first_missing_declared_dependency<type_list<Head, Tail...>,
                                         InterfaceBindings> {
  private:
    using dependency_type = binding_request_interface_t<Head>;
    using dependency_key = binding_request_key_t<Head>;

  public:
    using type = std::conditional_t<
        !declared_dependency_is_registered<Head, InterfaceBindings>::value,
        Head,
        typename first_missing_declared_dependency<type_list<Tail...>,
                                                   InterfaceBindings>::type>;
};

template <typename DependencyList, typename InterfaceBindings>
using first_missing_declared_dependency_t =
    typename first_missing_declared_dependency<DependencyList,
                                               InterfaceBindings>::type;

template <typename InterfaceBindings>
struct dependencies_registered<void, InterfaceBindings> : std::false_type {};

template <typename InterfaceBindings>
struct dependencies_registered<type_list<>, InterfaceBindings>
    : std::true_type {};

template <typename Head, typename... Tail, typename InterfaceBindings>
struct dependencies_registered<type_list<Head, Tail...>, InterfaceBindings>
    : std::bool_constant<
          declared_dependency_is_registered<Head, InterfaceBindings>::value &&
          dependencies_registered<type_list<Tail...>,
                                  InterfaceBindings>::value> {};

template <typename InterfaceBindings>
struct dependency_bindings<void, InterfaceBindings> {
    using type = void;
};

template <typename InterfaceBindings>
struct dependency_bindings<type_list<>, InterfaceBindings> {
    using type = type_list<>;
};

template <typename Dependency, typename InterfaceBindings,
          bool IsCollection = collection_traits<
              binding_request_interface_t<Dependency>>::is_collection>
struct dependency_binding_list;

template <typename Dependency, typename InterfaceBindings>
struct dependency_binding_list<Dependency, InterfaceBindings, false> {
  private:
    using dependency_type = binding_request_interface_t<Dependency>;
    using dependency_key = binding_request_key_t<Dependency>;

  public:
    using type =
        type_list<binding_t<dependency_type, dependency_key, InterfaceBindings>>;
};

template <typename Dependency, typename InterfaceBindings>
struct dependency_binding_list<Dependency, InterfaceBindings, true> {
  private:
    using dependency_type = binding_request_interface_t<Dependency>;
    using dependency_key = binding_request_key_t<Dependency>;
    using collection_type = collection_traits<dependency_type>;

  public:
    using type =
        bindings_t<normalized_type_t<typename collection_type::resolve_type>,
                   dependency_key, InterfaceBindings>;
};

template <typename... Dependencies, typename InterfaceBindings>
struct dependency_bindings<type_list<Dependencies...>, InterfaceBindings> {
    using type = type_list_cat_t<
        typename dependency_binding_list<Dependencies, InterfaceBindings>::type...>;
};

template <typename ResolvedBindings>
struct dependency_bindings_are_resolved : std::false_type {};

template <> struct dependency_bindings_are_resolved<void> : std::false_type {};

template <>
struct dependency_bindings_are_resolved<type_list<>> : std::true_type {};

template <typename Head, typename... Tail>
struct dependency_bindings_are_resolved<type_list<Head, Tail...>>
    : std::bool_constant<
          !std::is_void_v<Head> &&
          dependency_bindings_are_resolved<type_list<Tail...>>::value> {};

template <typename BindingModel, typename InterfaceBindings,
          typename LocalBindings = typename BindingModel::bindings_type>
struct effective_interface_bindings {
    using type = InterfaceBindings;
};

template <typename InterfaceBinding, typename InterfaceBindings>
struct binding_shadowed_by {
    static constexpr bool value = false;
};

template <typename InterfaceBinding, typename Head, typename... Tail>
struct binding_shadowed_by<InterfaceBinding, type_list<Head, Tail...>>
    : std::bool_constant<
          (std::is_same_v<typename InterfaceBinding::interface_type,
                          typename Head::interface_type> &&
           std::is_same_v<typename InterfaceBinding::key_type,
                          typename Head::key_type>) ||
          binding_shadowed_by<InterfaceBinding, type_list<Tail...>>::value> {};

template <typename InterfaceBindings, typename LocalInterfaceBindings>
struct remove_shadowed_bindings;

template <typename LocalInterfaceBindings>
struct remove_shadowed_bindings<type_list<>, LocalInterfaceBindings> {
    using type = type_list<>;
};

template <typename Head, typename... Tail, typename LocalInterfaceBindings>
struct remove_shadowed_bindings<type_list<Head, Tail...>,
                                LocalInterfaceBindings> {
  private:
    using tail_type =
        typename remove_shadowed_bindings<type_list<Tail...>,
                                          LocalInterfaceBindings>::type;

  public:
    using type = std::conditional_t<
        binding_shadowed_by<Head, LocalInterfaceBindings>::value, tail_type,
        type_list_cat_t<type_list<Head>, tail_type>>;
};

template <typename BindingModel, typename InterfaceBindings,
          typename... LocalRegistrations>
struct effective_interface_bindings<
    BindingModel, InterfaceBindings, static_registry<LocalRegistrations...>> {
  private:
    using local_interface_bindings = type_list_cat_t<
        typename binding_expansion<
            binding_model<LocalRegistrations>>::interface_bindings...>;
    using host_interface_bindings =
        typename remove_shadowed_bindings<InterfaceBindings,
                                          local_interface_bindings>::type;

  public:
    using type = type_list_cat_t<local_interface_bindings,
                                 host_interface_bindings>;
};

template <typename BindingModel, typename InterfaceBindings>
using effective_interface_bindings_t =
    typename effective_interface_bindings<BindingModel, InterfaceBindings>::type;

template <typename BindingModel, typename InterfaceBindings>
using resolved_dependency_bindings_t = typename binding_dependency_resolution<
    BindingModel,
    effective_interface_bindings_t<BindingModel, InterfaceBindings>>::type;

template <typename BindingModel, typename InterfaceBindings>
inline constexpr dependency_resolution_status
    binding_dependency_resolution_status_v =
        binding_dependency_resolution<
            BindingModel,
            effective_interface_bindings_t<BindingModel,
                                           InterfaceBindings>>::status;

template <typename BindingModel, typename StatusTag, typename = void>
struct inferred_dependency_problem_type {
    using type = void;
};

template <typename BindingModel>
struct inferred_dependency_problem_type<
    BindingModel,
    std::integral_constant<dependency_resolution_status,
                           dependency_resolution_status::kMissing>,
    std::enable_if_t<is_plain_constructor_factory<
        typename BindingModel::factory_type>::value>> {
    using type = typename constructor_factory_target<
        typename BindingModel::factory_type>::type;
};

template <typename BindingModel>
struct inferred_dependency_problem_type<
    BindingModel,
    std::integral_constant<dependency_resolution_status,
                           dependency_resolution_status::kAmbiguous>,
    std::enable_if_t<is_plain_constructor_factory<
        typename BindingModel::factory_type>::value>> {
    using type = typename constructor_factory_target<
        typename BindingModel::factory_type>::type;
};

template <typename BindingModel>
using inferred_missing_problem_type_t =
    typename inferred_dependency_problem_type<
        BindingModel,
        std::integral_constant<dependency_resolution_status,
                               dependency_resolution_status::kMissing>>::type;

template <typename BindingModel>
using inferred_ambiguous_problem_type_t =
    typename inferred_dependency_problem_type<
        BindingModel,
        std::integral_constant<dependency_resolution_status,
                               dependency_resolution_status::kAmbiguous>>::type;

template <typename BindingModel, typename MissingDependency,
          bool Valid = std::is_void_v<MissingDependency>>
struct declared_dependency_diagnostic;

template <typename BindingModel, typename MissingDependency>
struct declared_dependency_diagnostic<BindingModel, MissingDependency, true>
    : std::true_type {};

template <typename BindingModel, typename MissingDependency>
struct declared_dependency_diagnostic<BindingModel, MissingDependency, false> {
    static_assert(
        dependent_false_v<BindingModel, MissingDependency>,
        "bindings<...> source requires every declared dependency to map "
        "to an interface binding");
    static constexpr bool value = false;
};

template <typename BindingModel, typename InterfaceBindings,
          bool HasKnownDependencies =
              !std::is_same_v<binding_dependencies_t<BindingModel>, void>>
struct binding_declared_dependencies_resolved;

template <typename BindingModel, typename InterfaceBindings>
struct binding_declared_dependencies_resolved<BindingModel, InterfaceBindings,
                                              true>
    : std::bool_constant<std::is_void_v<first_missing_declared_dependency_t<
          binding_dependencies_t<BindingModel>,
          effective_interface_bindings_t<BindingModel, InterfaceBindings>>>> {};

template <typename BindingModel, typename InterfaceBindings>
struct binding_declared_dependencies_resolved<BindingModel, InterfaceBindings,
                                              false> : std::true_type {};

template <typename BindingModel, typename InterfaceBindings,
          bool DependenciesResolved = binding_declared_dependencies_resolved<
              BindingModel, InterfaceBindings>::value>
struct binding_declared_dependency_diagnostic;

template <typename BindingModel, typename InterfaceBindings>
struct binding_declared_dependency_diagnostic<BindingModel, InterfaceBindings,
                                              true> : std::true_type {};

template <typename BindingModel, typename InterfaceBindings>
struct binding_declared_dependency_diagnostic<BindingModel, InterfaceBindings,
                                              false>
    : declared_dependency_diagnostic<
          BindingModel,
          first_missing_declared_dependency_t<
              binding_dependencies_t<BindingModel>,
              effective_interface_bindings_t<BindingModel,
                                             InterfaceBindings>>> {};

template <typename BindingModel, typename InterfaceBindings,
          bool HasKnownDependencies =
              !std::is_same_v<binding_dependencies_t<BindingModel>, void>>
struct binding_inferred_dependencies_resolved;

template <typename BindingModel, typename ProblemType,
          bool Valid = std::is_void_v<ProblemType>>
struct inferred_missing_dependency_diagnostic;

template <typename BindingModel, typename ProblemType>
struct inferred_missing_dependency_diagnostic<BindingModel, ProblemType, true>
    : std::true_type {};

template <typename BindingModel, typename ProblemType>
struct inferred_missing_dependency_diagnostic<BindingModel, ProblemType,
                                              false> {
    static_assert(dependent_false_v<BindingModel, ProblemType>,
                  "bindings<...> source requires every inferred constructor "
                  "dependency to map to an interface binding");
    static constexpr bool value = false;
};

template <typename BindingModel, typename InterfaceBindings>
struct binding_inferred_dependencies_resolved<BindingModel, InterfaceBindings,
                                              true> : std::true_type {};

template <typename BindingModel, typename InterfaceBindings>
struct binding_inferred_dependencies_resolved<BindingModel, InterfaceBindings,
                                              false>
    : std::bool_constant<binding_dependency_resolution_status_v<
                             BindingModel, InterfaceBindings> !=
                         dependency_resolution_status::kMissing> {};

template <typename BindingModel, typename InterfaceBindings,
          bool DependenciesResolved = binding_inferred_dependencies_resolved<
              BindingModel, InterfaceBindings>::value>
struct binding_inferred_dependency_diagnostic;

template <typename BindingModel, typename InterfaceBindings>
struct binding_inferred_dependency_diagnostic<BindingModel, InterfaceBindings,
                                              true> : std::true_type {};

template <typename BindingModel, typename InterfaceBindings>
struct binding_inferred_dependency_diagnostic<BindingModel, InterfaceBindings,
                                              false>
    : inferred_missing_dependency_diagnostic<
          BindingModel,
          std::conditional_t<binding_dependency_resolution_status_v<
                                 BindingModel, InterfaceBindings> ==
                                 dependency_resolution_status::kMissing,
                             inferred_missing_problem_type_t<BindingModel>,
                             void>> {};

template <typename BindingModel, typename InterfaceBindings,
          bool HasKnownDependencies =
              !std::is_same_v<binding_dependencies_t<BindingModel>, void>>
struct binding_inferred_dependencies_unambiguous;

template <typename BindingModel, typename ProblemType,
          bool Valid = std::is_void_v<ProblemType>>
struct inferred_ambiguity_diagnostic;

template <typename BindingModel, typename ProblemType>
struct inferred_ambiguity_diagnostic<BindingModel, ProblemType, true>
    : std::true_type {};

template <typename BindingModel, typename ProblemType>
struct inferred_ambiguity_diagnostic<BindingModel, ProblemType, false> {
    static_assert(dependent_false_v<BindingModel, ProblemType>,
                  "bindings<...> source requires every inferred constructor "
                  "dependency to map to exactly one interface binding");
    static constexpr bool value = false;
};

template <typename BindingModel, typename InterfaceBindings>
struct binding_inferred_dependencies_unambiguous<BindingModel,
                                                 InterfaceBindings, true>
    : std::true_type {};

template <typename BindingModel, typename InterfaceBindings>
struct binding_inferred_dependencies_unambiguous<BindingModel,
                                                 InterfaceBindings, false>
    : std::bool_constant<binding_dependency_resolution_status_v<
                             BindingModel, InterfaceBindings> !=
                         dependency_resolution_status::kAmbiguous> {};

template <typename BindingModel, typename InterfaceBindings,
          bool DependenciesUnambiguous =
              binding_inferred_dependencies_unambiguous<
                  BindingModel, InterfaceBindings>::value>
struct binding_inferred_ambiguity_diagnostic;

template <typename BindingModel, typename InterfaceBindings>
struct binding_inferred_ambiguity_diagnostic<BindingModel, InterfaceBindings,
                                             true> : std::true_type {};

template <typename BindingModel, typename InterfaceBindings>
struct binding_inferred_ambiguity_diagnostic<BindingModel, InterfaceBindings,
                                             false>
    : inferred_ambiguity_diagnostic<
          BindingModel,
          std::conditional_t<binding_dependency_resolution_status_v<
                                 BindingModel, InterfaceBindings> ==
                                 dependency_resolution_status::kAmbiguous,
                             inferred_ambiguous_problem_type_t<BindingModel>,
                             void>> {};

template <typename BindingModel>
struct binding_factory_is_compile_time_bindable
    : std::bool_constant<factory_traits<
          typename BindingModel::factory_type>::is_compile_time_bindable> {};

template <typename... Registrations>
using static_registry_bindings_t =
    type_list_cat_t<typename binding_expansion<
        binding_model<Registrations>>::interface_bindings...>;

template <typename InterfaceBindings, typename... BindingModels>
struct static_registry_dependency_diagnostics
    // Keep dependency booleans available on the source itself so static-only
    // and static/runtime paths can branch on them. Emit the detailed
    // diagnostics only when a static graph path instantiates this helper.
    : binding_declared_dependency_diagnostic<BindingModels,
                                             InterfaceBindings>...,
      binding_inferred_dependency_diagnostic<BindingModels,
                                             InterfaceBindings>...,
      binding_inferred_ambiguity_diagnostic<BindingModels,
                                            InterfaceBindings>... {};



template <typename... Registrations> struct static_registry {
    using registration_types = type_list<Registrations...>;
    using binding_models = type_list<binding_model<Registrations>...>;
    using interface_bindings =
        static_registry_bindings_t<Registrations...>;

    static constexpr bool registrations_valid =
        (binding_model<Registrations>::valid && ...);
    static constexpr bool factories_are_compile_time_bindable =
        (binding_factory_is_compile_time_bindable<
             binding_model<Registrations>>::value &&
         ...);
    static constexpr bool declared_dependencies_are_resolved =
        (binding_declared_dependencies_resolved<
             binding_model<Registrations>, interface_bindings>::value &&
         ...);
    static constexpr bool inferred_dependencies_are_resolved =
        (binding_inferred_dependencies_resolved<
             binding_model<Registrations>, interface_bindings>::value &&
         ...);
    static constexpr bool inferred_dependencies_are_unambiguous =
        (binding_inferred_dependencies_unambiguous<
             binding_model<Registrations>, interface_bindings>::value &&
         ...);
    static constexpr bool dependencies_are_resolved =
        declared_dependencies_are_resolved &&
        inferred_dependencies_are_resolved &&
        inferred_dependencies_are_unambiguous;
    static constexpr bool valid =
        registrations_valid && factories_are_compile_time_bindable;

    static_assert(
        registrations_valid,
        "bindings<...> source requires valid compile-time registrations");
    static_assert(
        factories_are_compile_time_bindable,
        "bindings<...> source requires compile-time-bindable factories");
  private:
    template <typename Interface, typename Key = void>
    using binding_lookup_key_t =
        std::conditional_t<std::is_void_v<Key>,
                           binding_request_key_t<Interface>, Key>;

    template <typename Interface, typename Key = void>
    using exact_bindings_t = bindings_t<
        binding_exact_request_interface_t<Interface>,
        binding_lookup_key_t<Interface, Key>, interface_bindings>;

    template <typename Interface, typename Key = void>
    using normalized_bindings_t = bindings_t<
        binding_request_interface_t<Interface>,
        binding_lookup_key_t<Interface, Key>, interface_bindings>;

    template <typename Interface, typename Key = void,
              bool SameLookup =
                  std::is_same_v<binding_exact_request_interface_t<
                                     Interface>,
                                 binding_request_interface_t<Interface>>>
    struct selected_bindings {
        using exact = exact_bindings_t<Interface, Key>;
        using type =
            std::conditional_t<type_list_size_v<exact> != 0, exact,
                               normalized_bindings_t<Interface, Key>>;
    };

    template <typename Interface, typename Key>
    struct selected_bindings<Interface, Key, true> {
        using type = exact_bindings_t<Interface, Key>;
    };

  public:
    template <typename Interface, typename Key = void>
    using bindings = typename selected_bindings<Interface, Key>::type;

    template <typename Interface, typename Key = void>
    using binding = typename single_binding<bindings<Interface, Key>>::type;

    template <typename Interface>
    using model = typename binding<Interface>::binding_model_type;

    template <typename Interface>
    using dependencies = typename model<Interface>::dependencies_type::type;

    template <typename Interface>
    using dependency_bindings =
        resolved_dependency_bindings_t<model<Interface>,
                                               interface_bindings>;
};

} // export namespace silicon::di

// --- static/graph.h ---



export namespace silicon::di {

struct shared;
struct unique;
struct shared_cyclical;



template <typename Binding, typename DependencyBindings> struct graph_node {
    using binding_type = Binding;
    using dependency_bindings = DependencyBindings;
};

template <typename DependencyBindings>
struct filter_resolved_dependency_bindings;

template <> struct filter_resolved_dependency_bindings<void> {
    using type = type_list<>;
};

template <> struct filter_resolved_dependency_bindings<type_list<>> {
    using type = type_list<>;
};

template <typename Head, typename... Tail>
struct filter_resolved_dependency_bindings<type_list<Head, Tail...>> {
  private:
    using tail_type =
        typename filter_resolved_dependency_bindings<type_list<Tail...>>::type;

  public:
    using type =
        std::conditional_t<std::is_void_v<Head>, tail_type,
                           type_list_cat_t<type_list<Head>, tail_type>>;
};

template <typename DependencyBindings>
using filter_resolved_dependency_bindings_t =
    typename filter_resolved_dependency_bindings<DependencyBindings>::type;

template <typename InterfaceBinding, typename StaticRegistry,
          bool RuntimeDependencies = false>
struct static_graph_node {
    using binding_model_type = typename InterfaceBinding::binding_model_type;
    using resolved_dependency_bindings = resolved_dependency_bindings_t<
        binding_model_type, typename StaticRegistry::interface_bindings>;
    using type = graph_node<
        InterfaceBinding,
        resolved_dependency_bindings_t<
            binding_model_type, typename StaticRegistry::interface_bindings>>;
};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_node<void, StaticRegistry, RuntimeDependencies> {
    using type = void;
};

template <typename InterfaceBinding, typename StaticRegistry>
struct static_graph_node<InterfaceBinding, StaticRegistry, true> {
    using binding_model_type = typename InterfaceBinding::binding_model_type;
    using resolved_dependency_bindings = resolved_dependency_bindings_t<
        binding_model_type, typename StaticRegistry::interface_bindings>;
    using type = graph_node<
        InterfaceBinding,
        filter_resolved_dependency_bindings_t<resolved_dependency_bindings>>;
};

template <typename InterfaceBinding, typename StaticRegistry,
          bool RuntimeDependencies = false>
using static_graph_node_t =
    typename static_graph_node<InterfaceBinding, StaticRegistry,
                               RuntimeDependencies>::type;

template <typename Bindings, typename StaticRegistry,
          typename Visiting = type_list<>>
struct static_bindings_resolvable;

template <typename Binding, typename StaticRegistry,
          typename Visiting = type_list<>,
          bool InVisiting = type_list_contains_v<Binding, Visiting>>
struct static_binding_resolvable;

template <typename Binding>
struct static_binding_uses_cyclical_storage
    : std::is_same<typename Binding::binding_model_type::storage_tag,
                   shared_cyclical> {};

template <typename Binding>
inline constexpr bool static_binding_uses_cyclical_storage_v =
    static_binding_uses_cyclical_storage<Binding>::value;

template <typename Binding, typename Visiting> struct static_cycle_path;

template <typename Binding> struct static_cycle_path<Binding, type_list<>> {
    using type = type_list<>;
};

template <typename Binding, typename Head, typename... Tail>
struct static_cycle_path<Binding, type_list<Head, Tail...>> {
    using tail_path =
        typename static_cycle_path<Binding, type_list<Tail...>>::type;
    using type = std::conditional_t<std::is_same_v<Binding, Head>,
                                    type_list<Head, Tail...>, tail_path>;
};

template <typename Binding, typename Visiting>
using static_cycle_path_t = typename static_cycle_path<Binding, Visiting>::type;

template <typename Bindings> struct static_bindings_use_cyclical_storage;

template <>
struct static_bindings_use_cyclical_storage<type_list<>> : std::true_type {};

template <typename... Bindings>
struct static_bindings_use_cyclical_storage<type_list<Bindings...>>
    : std::bool_constant<(static_binding_uses_cyclical_storage_v<Bindings> &&
                          ...)> {};

template <typename Binding, typename Visiting>
inline constexpr bool static_cycle_uses_cyclical_storage_v =
    static_bindings_use_cyclical_storage<
        static_cycle_path_t<Binding, Visiting>>::value;

template <typename StaticRegistry, typename Visiting>
struct static_binding_resolvable<void, StaticRegistry, Visiting, false>
    : std::false_type {};

template <typename Binding, typename StaticRegistry, typename Visiting>
struct static_binding_resolvable<Binding, StaticRegistry, Visiting, true>
    : std::bool_constant<
          static_cycle_uses_cyclical_storage_v<Binding, Visiting>> {};

template <typename Binding, typename StaticRegistry, typename Visiting>
struct static_binding_resolvable<Binding, StaticRegistry, Visiting, false>
    : static_bindings_resolvable<
          typename static_graph_node_t<Binding, StaticRegistry,
                                       false>::dependency_bindings,
          StaticRegistry, type_list_cat_t<Visiting, type_list<Binding>>> {};

template <typename StaticRegistry, typename Visiting>
struct static_bindings_resolvable<type_list<>, StaticRegistry, Visiting>
    : std::true_type {};

template <typename StaticRegistry, typename Visiting>
struct static_bindings_resolvable<void, StaticRegistry, Visiting>
    : std::false_type {};

template <typename... Bindings, typename StaticRegistry, typename Visiting>
struct static_bindings_resolvable<type_list<Bindings...>, StaticRegistry,
                                  Visiting>
    : std::bool_constant<(static_binding_resolvable<Bindings, StaticRegistry,
                                                    Visiting>::value &&
                          ...)> {};

template <typename Binding, typename StaticRegistry>
inline constexpr bool static_binding_resolvable_v =
    static_binding_resolvable<Binding, StaticRegistry>::value;

template <typename Bindings, typename StaticRegistry>
inline constexpr bool static_bindings_resolvable_v =
    static_bindings_resolvable<Bindings, StaticRegistry>::value;

template <typename InterfaceBindings, typename StaticRegistry,
          bool RuntimeDependencies = false>
struct static_graph_nodes;

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_nodes<type_list<>, StaticRegistry, RuntimeDependencies> {
    using type = type_list<>;
};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_nodes<void, StaticRegistry, RuntimeDependencies> {
    using type = void;
};

template <typename... InterfaceBindings, typename StaticRegistry,
          bool RuntimeDependencies>
struct static_graph_nodes<type_list<InterfaceBindings...>, StaticRegistry,
                          RuntimeDependencies> {
    using type =
        type_list<static_graph_node_t<InterfaceBindings, StaticRegistry,
                                      RuntimeDependencies>...>;
};

template <typename InterfaceBindings, typename StaticRegistry,
          bool RuntimeDependencies = false>
using static_graph_nodes_t =
    typename static_graph_nodes<InterfaceBindings, StaticRegistry,
                                RuntimeDependencies>::type;

template <typename DependencyBindings, typename StaticRegistry>
struct dependency_graph_nodes;

template <typename StaticRegistry>
struct dependency_graph_nodes<void, StaticRegistry> {
    using type = void;
};

template <typename StaticRegistry>
struct dependency_graph_nodes<type_list<>, StaticRegistry> {
    using type = type_list<>;
};

template <typename... DependencyBindings, typename StaticRegistry>
struct dependency_graph_nodes<type_list<DependencyBindings...>,
                              StaticRegistry> {
    using type =
        type_list<static_graph_node_t<DependencyBindings, StaticRegistry>...>;
};

template <typename DependencyBindings, typename StaticRegistry>
using dependency_graph_nodes_t =
    typename dependency_graph_nodes<DependencyBindings, StaticRegistry>::type;

template <typename StorageTag>
struct static_preserves_closure : std::false_type {};

template <> struct static_preserves_closure<shared> : std::true_type {};

template <typename StorageTag>
inline constexpr bool static_preserves_closure_v =
    static_preserves_closure<StorageTag>::value;

template <typename StorageTag>
struct static_storage_rollback_cost : std::integral_constant<std::size_t, 0> {};

template <>
struct static_storage_rollback_cost<shared_cyclical>
    : std::integral_constant<std::size_t, 1> {};

template <typename StorageTag>
inline constexpr std::size_t static_storage_rollback_cost_v =
    static_storage_rollback_cost<StorageTag>::value;

template <typename StorageTag>
struct static_storage_temporary_slot_cost
    : std::integral_constant<std::size_t, 0> {};

template <>
struct static_storage_temporary_slot_cost<unique>
    : std::integral_constant<std::size_t, 1> {};

template <>
struct static_storage_temporary_slot_cost<shared_cyclical>
    : std::integral_constant<std::size_t, 1> {};

template <typename StorageTag>
inline constexpr std::size_t static_storage_temporary_slot_cost_v =
    static_storage_temporary_slot_cost<StorageTag>::value;

template <typename StorageTag>
struct static_storage_temporary_size : std::integral_constant<std::size_t, 0> {
};

template <>
struct static_storage_temporary_size<shared_cyclical>
    : std::integral_constant<std::size_t, sizeof(void*)> {};

template <typename StorageTag>
inline constexpr std::size_t static_storage_temporary_size_v =
    static_storage_temporary_size<StorageTag>::value;

template <typename StorageTag>
struct static_storage_temporary_align : std::integral_constant<std::size_t, 0> {
};

template <>
struct static_storage_temporary_align<shared_cyclical>
    : std::integral_constant<std::size_t, alignof(void*)> {};

template <typename StorageTag>
inline constexpr std::size_t static_storage_temporary_align_v =
    static_storage_temporary_align<StorageTag>::value;

template <typename Request>
using static_request_type_t = std::remove_cv_t<
    std::remove_reference_t<typename annotated_traits<Request>::type>>;

template <typename Request>
inline constexpr bool static_request_uses_temporary_slot_v =
    !std::is_reference_v<typename annotated_traits<Request>::type> &&
    !std::is_pointer_v<typename annotated_traits<Request>::type>;

template <typename Request>
inline constexpr std::size_t static_request_temporary_slot_cost_v =
    static_request_uses_temporary_slot_v<Request> ? 1 : 0;

template <typename Request>
inline constexpr std::size_t static_request_temporary_size_v =
    static_request_uses_temporary_slot_v<Request>
        ? sizeof(static_request_type_t<Request>)
        : 0;

template <typename Request>
inline constexpr std::size_t static_request_temporary_align_v =
    static_request_uses_temporary_slot_v<Request>
        ? alignof(static_request_type_t<Request>)
        : 0;

template <typename Request>
inline constexpr std::size_t static_request_destructible_cost_v =
    !std::is_trivially_destructible_v<
        std::remove_cv_t<std::remove_reference_t<Request>>>
        ? 1
        : 0;

template <typename InterfaceBinding>
inline constexpr bool static_binding_is_stable_v =
    InterfaceBinding::binding_model_type::storage_type::conversions::is_stable;

template <typename InterfaceBinding>
inline constexpr std::size_t static_binding_self_temporary_slot_cost_v =
    std::max({static_conversion_temporary_slots_v<
                  typename InterfaceBinding::binding_model_type::storage_type>,
              static_storage_temporary_slot_cost_v<
                  typename InterfaceBinding::binding_model_type::storage_tag>,
              std::size_t{0}});

template <typename InterfaceBinding>
inline constexpr std::size_t static_binding_self_destructible_slot_cost_v =
    std::max(
        {static_conversion_destructible_slots_v<
             typename InterfaceBinding::binding_model_type::storage_type>,
         static_binding_self_temporary_slot_cost_v<InterfaceBinding> != 0 &&
                 !std::is_trivially_destructible_v<
                     typename InterfaceBinding::binding_model_type::
                         storage_type::type>
             ? std::size_t{1}
             : std::size_t{0},
         static_storage_rollback_cost_v<
             typename InterfaceBinding::binding_model_type::storage_tag>,
         std::size_t{0}});

template <typename Dependencies> struct static_dependency_destructible_cost;

template <typename Dependencies> struct static_dependency_temporary_slot_cost;

template <typename Dependencies> struct static_dependency_temporary_size;

template <typename Dependencies> struct static_dependency_temporary_align;

template <typename Requests, typename Bindings, typename StaticRegistry>
struct static_dependency_retained_destructible_slots;

template <typename Requests, typename Bindings, typename StaticRegistry>
struct static_dependency_peak_destructible_slots;

template <typename Request, typename Binding, typename StaticRegistry>
struct static_request_retained_destructible_slots;

template <typename Request, typename Binding, typename StaticRegistry>
struct static_request_peak_destructible_slots;

template <typename Binding, typename StaticRegistry>
struct static_binding_retained_destructible_slots;

template <typename Binding, typename StaticRegistry>
struct static_binding_peak_destructible_slots;

template <typename Requests, typename Bindings, typename StaticRegistry>
struct static_dependency_retained_temporary_slots;

template <typename Requests, typename Bindings, typename StaticRegistry>
struct static_dependency_peak_temporary_slots;

template <typename Request, typename Binding, typename StaticRegistry>
struct static_request_retained_temporary_slots;

template <typename Request, typename Binding, typename StaticRegistry>
struct static_request_peak_temporary_slots;

template <typename Binding, typename StaticRegistry>
struct static_binding_retained_temporary_slots;

template <typename Binding, typename StaticRegistry>
struct static_binding_peak_temporary_slots;

template <typename Request, typename StaticRegistry>
struct static_request_retained_destructible_slots<Request, void, StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename Request, typename StaticRegistry>
struct static_request_peak_destructible_slots<Request, void, StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry>
struct static_binding_retained_destructible_slots<void, StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry>
struct static_binding_peak_destructible_slots<void, StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename Request, typename StaticRegistry>
struct static_request_retained_temporary_slots<Request, void, StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename Request, typename StaticRegistry>
struct static_request_peak_temporary_slots<Request, void, StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry>
struct static_binding_retained_temporary_slots<void, StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry>
struct static_binding_peak_temporary_slots<void, StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <>
struct static_dependency_destructible_cost<void>
    : std::integral_constant<std::size_t, 0> {};

template <>
struct static_dependency_destructible_cost<type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Dependencies>
struct static_dependency_destructible_cost<type_list<Dependencies...>>
    : std::integral_constant<std::size_t,
                             (static_request_destructible_cost_v<Dependencies> +
                              ... + std::size_t{0})> {};

template <>
struct static_dependency_temporary_slot_cost<void>
    : std::integral_constant<std::size_t, 0> {};

template <>
struct static_dependency_temporary_slot_cost<type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Dependencies>
struct static_dependency_temporary_slot_cost<type_list<Dependencies...>>
    : std::integral_constant<
          std::size_t, (static_request_temporary_slot_cost_v<Dependencies> +
                        ... + std::size_t{0})> {};

template <>
struct static_dependency_temporary_size<void>
    : std::integral_constant<std::size_t, 0> {};

template <>
struct static_dependency_temporary_size<type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Dependencies>
struct static_dependency_temporary_size<type_list<Dependencies...>>
    : std::integral_constant<
          std::size_t,
          std::max({static_request_temporary_size_v<Dependencies>...,
                    std::size_t{0}})> {};

template <>
struct static_dependency_temporary_align<void>
    : std::integral_constant<std::size_t, 0> {};

template <>
struct static_dependency_temporary_align<type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Dependencies>
struct static_dependency_temporary_align<type_list<Dependencies...>>
    : std::integral_constant<
          std::size_t,
          std::max({static_request_temporary_align_v<Dependencies>...,
                    std::size_t{0}})> {};

template <typename InterfaceBinding>
inline constexpr std::size_t static_binding_preserved_closure_cost_v =
    static_preserves_closure_v<
        typename InterfaceBinding::binding_model_type::storage_tag>
        ? 1
        : 0;

template <typename InterfaceBinding>
inline constexpr std::size_t static_binding_destructible_slot_cost_v =
    static_dependency_destructible_cost<
        typename InterfaceBinding::binding_model_type::dependencies_type::
            type>::value +
    static_conversion_destructible_slots_v<
        typename InterfaceBinding::binding_model_type::storage_type> +
    (!std::is_trivially_destructible_v<
         typename InterfaceBinding::binding_model_type::storage_type::type>
         ? 1
         : 0) +
    static_storage_rollback_cost_v<
        typename InterfaceBinding::binding_model_type::storage_tag>;

template <typename InterfaceBinding>
inline constexpr std::size_t static_binding_temporary_slot_cost_v =
    static_dependency_temporary_slot_cost<
        typename InterfaceBinding::binding_model_type::dependencies_type::
            type>::value +
    static_conversion_temporary_slots_v<
        typename InterfaceBinding::binding_model_type::storage_type> +
    static_storage_temporary_slot_cost_v<
        typename InterfaceBinding::binding_model_type::storage_tag>;

template <typename InterfaceBinding>
inline constexpr std::size_t static_binding_max_temporary_size_v = std::max(
    {static_dependency_temporary_size<
         typename InterfaceBinding::binding_model_type::dependencies_type::
             type>::value,
     static_conversion_temporary_size_v<
         typename InterfaceBinding::binding_model_type::storage_type>,
     sizeof(typename InterfaceBinding::binding_model_type::storage_type::type),
     static_storage_temporary_size_v<
         typename InterfaceBinding::binding_model_type::storage_tag>,
     std::size_t{0}});

template <typename InterfaceBinding>
inline constexpr std::size_t static_binding_max_temporary_align_v = std::max(
    {static_dependency_temporary_align<
         typename InterfaceBinding::binding_model_type::dependencies_type::
             type>::value,
     static_conversion_temporary_align_v<
         typename InterfaceBinding::binding_model_type::storage_type>,
     alignof(typename InterfaceBinding::binding_model_type::storage_type::type),
     static_storage_temporary_align_v<
         typename InterfaceBinding::binding_model_type::storage_tag>,
     std::size_t{0}});

template <typename StaticRegistry>
struct static_dependency_retained_destructible_slots<type_list<>, type_list<>,
                                                     StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename Bindings, typename StaticRegistry>
struct static_dependency_retained_destructible_slots<void, Bindings,
                                                     StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename RequestHead, typename... RequestTail, typename BindingHead,
          typename... BindingTail, typename StaticRegistry>
struct static_dependency_retained_destructible_slots<
    type_list<RequestHead, RequestTail...>,
    type_list<BindingHead, BindingTail...>, StaticRegistry>
    : std::integral_constant<
          std::size_t,
          static_request_retained_destructible_slots<RequestHead, BindingHead,
                                                     StaticRegistry>::value +
              static_dependency_retained_destructible_slots<
                  type_list<RequestTail...>, type_list<BindingTail...>,
                  StaticRegistry>::value> {};

template <typename StaticRegistry>
struct static_dependency_peak_destructible_slots<type_list<>, type_list<>,
                                                 StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename Bindings, typename StaticRegistry>
struct static_dependency_peak_destructible_slots<void, Bindings, StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename RequestHead, typename... RequestTail, typename BindingHead,
          typename... BindingTail, typename StaticRegistry>
struct static_dependency_peak_destructible_slots<
    type_list<RequestHead, RequestTail...>,
    type_list<BindingHead, BindingTail...>, StaticRegistry>
    : std::integral_constant<
          std::size_t,
          std::max(static_request_peak_destructible_slots<
                       RequestHead, BindingHead, StaticRegistry>::value +
                       static_dependency_retained_destructible_slots<
                           type_list<RequestTail...>, type_list<BindingTail...>,
                           StaticRegistry>::value,
                   static_request_retained_destructible_slots<
                       RequestHead, BindingHead, StaticRegistry>::value +
                       static_dependency_peak_destructible_slots<
                           type_list<RequestTail...>, type_list<BindingTail...>,
                           StaticRegistry>::value)> {};

template <typename Request, typename Binding, typename StaticRegistry>
struct static_request_retained_destructible_slots
    : std::integral_constant<
          std::size_t, static_binding_retained_destructible_slots<
                           Binding, StaticRegistry>::value +
                           (static_request_uses_temporary_slot_v<Request> &&
                                    static_binding_is_stable_v<Binding>
                                ? static_request_destructible_cost_v<Request>
                                : std::size_t{0})> {};

template <typename Request, typename Binding, typename StaticRegistry>
struct static_request_peak_destructible_slots
    : std::integral_constant<
          std::size_t, std::max(static_binding_peak_destructible_slots<
                                    Binding, StaticRegistry>::value,
                                static_request_retained_destructible_slots<
                                    Request, Binding, StaticRegistry>::value)> {
};

template <typename Binding, typename StaticRegistry>
struct static_binding_retained_destructible_slots
    : std::integral_constant<
          std::size_t,
          static_dependency_retained_destructible_slots<
              typename Binding::binding_model_type::dependencies_type::type,
              typename static_graph_node_t<Binding, StaticRegistry,
                                           false>::dependency_bindings,
              StaticRegistry>::value +
              static_binding_self_destructible_slot_cost_v<Binding>> {};

template <typename Binding, typename StaticRegistry>
struct static_binding_peak_destructible_slots
    : std::integral_constant<
          std::size_t,
          std::max(
              static_dependency_peak_destructible_slots<
                  typename Binding::binding_model_type::dependencies_type::type,
                  typename static_graph_node_t<Binding, StaticRegistry,
                                               false>::dependency_bindings,
                  StaticRegistry>::value,
              static_binding_retained_destructible_slots<
                  Binding, StaticRegistry>::value)> {};

template <typename StaticRegistry>
struct static_dependency_retained_temporary_slots<type_list<>, type_list<>,
                                                  StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename Bindings, typename StaticRegistry>
struct static_dependency_retained_temporary_slots<void, Bindings,
                                                  StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename RequestHead, typename... RequestTail, typename BindingHead,
          typename... BindingTail, typename StaticRegistry>
struct static_dependency_retained_temporary_slots<
    type_list<RequestHead, RequestTail...>,
    type_list<BindingHead, BindingTail...>, StaticRegistry>
    : std::integral_constant<
          std::size_t,
          static_request_retained_temporary_slots<RequestHead, BindingHead,
                                                  StaticRegistry>::value +
              static_dependency_retained_temporary_slots<
                  type_list<RequestTail...>, type_list<BindingTail...>,
                  StaticRegistry>::value> {};

template <typename StaticRegistry>
struct static_dependency_peak_temporary_slots<type_list<>, type_list<>,
                                              StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename Bindings, typename StaticRegistry>
struct static_dependency_peak_temporary_slots<void, Bindings, StaticRegistry>
    : std::integral_constant<std::size_t, 0> {};

template <typename RequestHead, typename... RequestTail, typename BindingHead,
          typename... BindingTail, typename StaticRegistry>
struct static_dependency_peak_temporary_slots<
    type_list<RequestHead, RequestTail...>,
    type_list<BindingHead, BindingTail...>, StaticRegistry>
    : std::integral_constant<
          std::size_t,
          std::max(static_request_peak_temporary_slots<RequestHead, BindingHead,
                                                       StaticRegistry>::value +
                       static_dependency_retained_temporary_slots<
                           type_list<RequestTail...>, type_list<BindingTail...>,
                           StaticRegistry>::value,
                   static_request_retained_temporary_slots<
                       RequestHead, BindingHead, StaticRegistry>::value +
                       static_dependency_peak_temporary_slots<
                           type_list<RequestTail...>, type_list<BindingTail...>,
                           StaticRegistry>::value)> {};

template <typename Request, typename Binding, typename StaticRegistry>
struct static_request_retained_temporary_slots
    : std::integral_constant<
          std::size_t, static_binding_retained_temporary_slots<
                           Binding, StaticRegistry>::value +
                           (static_request_uses_temporary_slot_v<Request> &&
                                    static_binding_is_stable_v<Binding>
                                ? std::size_t{1}
                                : std::size_t{0})> {};

template <typename Request, typename Binding, typename StaticRegistry>
struct static_request_peak_temporary_slots
    : std::integral_constant<
          std::size_t, std::max(static_binding_peak_temporary_slots<
                                    Binding, StaticRegistry>::value,
                                static_request_retained_temporary_slots<
                                    Request, Binding, StaticRegistry>::value)> {
};

template <typename Binding, typename StaticRegistry>
struct static_binding_retained_temporary_slots
    : std::integral_constant<
          std::size_t,
          static_dependency_retained_temporary_slots<
              typename Binding::binding_model_type::dependencies_type::type,
              typename static_graph_node_t<Binding, StaticRegistry,
                                           false>::dependency_bindings,
              StaticRegistry>::value +
              static_binding_self_temporary_slot_cost_v<Binding>> {};

template <typename Binding, typename StaticRegistry>
struct static_binding_peak_temporary_slots
    : std::integral_constant<
          std::size_t,
          std::max(
              static_dependency_peak_temporary_slots<
                  typename Binding::binding_model_type::dependencies_type::type,
                  typename static_graph_node_t<Binding, StaticRegistry,
                                               false>::dependency_bindings,
                  StaticRegistry>::value,
              static_binding_retained_temporary_slots<Binding,
                                                      StaticRegistry>::value)> {
};

template <typename Bindings, typename StaticRegistry,
          bool RuntimeDependencies = false>
struct static_graph_max_preserved_closure_depth_all;

template <typename StaticRegistry, bool RuntimeDependencies, bool Acyclic>
struct static_graph_max_preserved_closure_depth;

template <typename Binding, typename StaticRegistry,
          bool RuntimeDependencies = false>
struct static_binding_graph_preserved_closure_depth;

template <typename Bindings, typename StaticRegistry,
          bool RuntimeDependencies = false>
struct static_graph_max_destructible_slots_all;

template <typename StaticRegistry, bool RuntimeDependencies, bool Acyclic>
struct static_graph_max_destructible_slots;

template <typename Binding, typename StaticRegistry,
          bool RuntimeDependencies = false>
struct static_binding_graph_destructible_slots;

template <typename Bindings> struct static_graph_max_temporary_size_all;

template <typename StaticRegistry, bool Acyclic>
struct static_graph_max_temporary_size;

template <typename Bindings> struct static_graph_max_temporary_align_all;

template <typename StaticRegistry, bool Acyclic>
struct static_graph_max_temporary_align;

template <typename Bindings, typename StaticRegistry,
          bool RuntimeDependencies = false>
struct static_graph_max_temporary_slots_all;

template <typename StaticRegistry, bool RuntimeDependencies, bool Acyclic>
struct static_graph_max_temporary_slots;

template <typename Binding, typename StaticRegistry,
          bool RuntimeDependencies = false>
struct static_binding_graph_temporary_slots;

template <typename Bindings> struct static_graph_total_preserved_closure_depth;

template <typename Bindings> struct static_graph_total_destructible_slots;

template <typename Bindings> struct static_graph_total_temporary_slots;

template <typename StaticRegistry, bool RuntimeDependencies, bool Acyclic,
          bool ContainsCycle>
struct static_graph_preserved_closure_depth_bound;

template <typename StaticRegistry, bool RuntimeDependencies, bool Acyclic,
          bool ContainsCycle>
struct static_graph_destructible_slots_bound;

template <typename StaticRegistry, bool RuntimeDependencies, bool Acyclic,
          bool ContainsCycle>
struct static_graph_temporary_slots_bound;

template <>
struct static_graph_total_preserved_closure_depth<type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Bindings>
struct static_graph_total_preserved_closure_depth<type_list<Bindings...>>
    : std::integral_constant<
          std::size_t, (static_binding_preserved_closure_cost_v<Bindings> +
                        ... + std::size_t{0})> {};

template <>
struct static_graph_total_destructible_slots<type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Bindings>
struct static_graph_total_destructible_slots<type_list<Bindings...>>
    : std::integral_constant<
          std::size_t, (static_binding_destructible_slot_cost_v<Bindings> +
                        ... + std::size_t{0})> {};

template <>
struct static_graph_total_temporary_slots<type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Bindings>
struct static_graph_total_temporary_slots<type_list<Bindings...>>
    : std::integral_constant<std::size_t,
                             (static_binding_temporary_slot_cost_v<Bindings> +
                              ... + std::size_t{0})> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_preserved_closure_depth_all<type_list<>, StaticRegistry,
                                                    RuntimeDependencies>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_preserved_closure_depth_all<void, StaticRegistry,
                                                    RuntimeDependencies>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Bindings, typename StaticRegistry,
          bool RuntimeDependencies>
struct static_graph_max_preserved_closure_depth_all<
    type_list<Bindings...>, StaticRegistry, RuntimeDependencies>
    : std::integral_constant<
          std::size_t, std::max({static_binding_graph_preserved_closure_depth<
                                     Bindings, StaticRegistry,
                                     RuntimeDependencies>::value...,
                                 std::size_t{0}})> {};

template <typename Binding, typename StaticRegistry, bool RuntimeDependencies>
struct static_binding_graph_preserved_closure_depth
    : std::integral_constant<
          std::size_t, static_binding_preserved_closure_cost_v<Binding> +
                           static_graph_max_preserved_closure_depth_all<
                               typename static_graph_node_t<
                                   Binding, StaticRegistry,
                                   RuntimeDependencies>::dependency_bindings,
                               StaticRegistry, RuntimeDependencies>::value> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_binding_graph_preserved_closure_depth<void, StaticRegistry,
                                                    RuntimeDependencies>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_destructible_slots_all<type_list<>, StaticRegistry,
                                               RuntimeDependencies>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_destructible_slots_all<void, StaticRegistry,
                                               RuntimeDependencies>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Bindings, typename StaticRegistry,
          bool RuntimeDependencies>
struct static_graph_max_destructible_slots_all<
    type_list<Bindings...>, StaticRegistry, RuntimeDependencies>
    : std::integral_constant<std::size_t,
                             std::max({static_binding_graph_destructible_slots<
                                           Bindings, StaticRegistry,
                                           RuntimeDependencies>::value...,
                                       std::size_t{0}})> {};

template <typename Binding, typename StaticRegistry, bool RuntimeDependencies>
struct static_binding_graph_destructible_slots
    : std::integral_constant<
          std::size_t, static_binding_destructible_slot_cost_v<Binding> +
                           static_graph_max_destructible_slots_all<
                               typename static_graph_node_t<
                                   Binding, StaticRegistry,
                                   RuntimeDependencies>::dependency_bindings,
                               StaticRegistry, RuntimeDependencies>::value> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_binding_graph_destructible_slots<void, StaticRegistry,
                                               RuntimeDependencies>
    : std::integral_constant<std::size_t, 0> {};

template <typename Binding, typename StaticRegistry>
struct static_binding_graph_destructible_slots<Binding, StaticRegistry, false>
    : static_binding_peak_destructible_slots<Binding, StaticRegistry> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_preserved_closure_depth<StaticRegistry,
                                                RuntimeDependencies, false>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_preserved_closure_depth<StaticRegistry,
                                                RuntimeDependencies, true>
    : static_graph_max_preserved_closure_depth_all<
          typename StaticRegistry::interface_bindings, StaticRegistry,
          RuntimeDependencies> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_destructible_slots<StaticRegistry, RuntimeDependencies,
                                           false>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_destructible_slots<StaticRegistry, RuntimeDependencies,
                                           true>
    : static_graph_max_destructible_slots_all<
          typename StaticRegistry::interface_bindings, StaticRegistry,
          RuntimeDependencies> {};

template <>
struct static_graph_max_temporary_size_all<type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Bindings>
struct static_graph_max_temporary_size_all<type_list<Bindings...>>
    : std::integral_constant<
          std::size_t,
          std::max({static_binding_max_temporary_size_v<Bindings>...,
                    std::size_t{0}})> {};

template <typename StaticRegistry>
struct static_graph_max_temporary_size<StaticRegistry, false>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry>
struct static_graph_max_temporary_size<StaticRegistry, true>
    : static_graph_max_temporary_size_all<
          typename StaticRegistry::interface_bindings> {};

template <>
struct static_graph_max_temporary_align_all<type_list<>>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Bindings>
struct static_graph_max_temporary_align_all<type_list<Bindings...>>
    : std::integral_constant<
          std::size_t,
          std::max({static_binding_max_temporary_align_v<Bindings>...,
                    std::size_t{0}})> {};

template <typename StaticRegistry>
struct static_graph_max_temporary_align<StaticRegistry, false>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry>
struct static_graph_max_temporary_align<StaticRegistry, true>
    : static_graph_max_temporary_align_all<
          typename StaticRegistry::interface_bindings> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_temporary_slots_all<type_list<>, StaticRegistry,
                                            RuntimeDependencies>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_temporary_slots_all<void, StaticRegistry,
                                            RuntimeDependencies>
    : std::integral_constant<std::size_t, 0> {};

template <typename... Bindings, typename StaticRegistry,
          bool RuntimeDependencies>
struct static_graph_max_temporary_slots_all<type_list<Bindings...>,
                                            StaticRegistry, RuntimeDependencies>
    : std::integral_constant<std::size_t,
                             std::max({static_binding_graph_temporary_slots<
                                           Bindings, StaticRegistry,
                                           RuntimeDependencies>::value...,
                                       std::size_t{0}})> {};

template <typename Binding, typename StaticRegistry, bool RuntimeDependencies>
struct static_binding_graph_temporary_slots
    : std::integral_constant<
          std::size_t, static_binding_temporary_slot_cost_v<Binding> +
                           static_graph_max_temporary_slots_all<
                               typename static_graph_node_t<
                                   Binding, StaticRegistry,
                                   RuntimeDependencies>::dependency_bindings,
                               StaticRegistry, RuntimeDependencies>::value> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_binding_graph_temporary_slots<void, StaticRegistry,
                                            RuntimeDependencies>
    : std::integral_constant<std::size_t, 0> {};

template <typename Binding, typename StaticRegistry>
struct static_binding_graph_temporary_slots<Binding, StaticRegistry, false>
    : static_binding_peak_temporary_slots<Binding, StaticRegistry> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_temporary_slots<StaticRegistry, RuntimeDependencies,
                                        false>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_max_temporary_slots<StaticRegistry, RuntimeDependencies,
                                        true>
    : static_graph_max_temporary_slots_all<
          typename StaticRegistry::interface_bindings, StaticRegistry,
          RuntimeDependencies> {};

template <typename StaticRegistry, bool RuntimeDependencies, bool ContainsCycle>
struct static_graph_preserved_closure_depth_bound<
    StaticRegistry, RuntimeDependencies, false, ContainsCycle>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_preserved_closure_depth_bound<
    StaticRegistry, RuntimeDependencies, true, false>
    : static_graph_max_preserved_closure_depth<StaticRegistry,
                                               RuntimeDependencies, true> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_preserved_closure_depth_bound<
    StaticRegistry, RuntimeDependencies, true, true>
    : static_graph_total_preserved_closure_depth<
          typename StaticRegistry::interface_bindings> {};

template <typename StaticRegistry, bool RuntimeDependencies, bool ContainsCycle>
struct static_graph_destructible_slots_bound<
    StaticRegistry, RuntimeDependencies, false, ContainsCycle>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_destructible_slots_bound<StaticRegistry,
                                             RuntimeDependencies, true, false>
    : static_graph_max_destructible_slots<StaticRegistry, RuntimeDependencies,
                                          true> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_destructible_slots_bound<StaticRegistry,
                                             RuntimeDependencies, true, true>
    : static_graph_total_destructible_slots<
          typename StaticRegistry::interface_bindings> {};

template <typename StaticRegistry, bool RuntimeDependencies, bool ContainsCycle>
struct static_graph_temporary_slots_bound<StaticRegistry, RuntimeDependencies,
                                          false, ContainsCycle>
    : std::integral_constant<std::size_t, 0> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_temporary_slots_bound<StaticRegistry, RuntimeDependencies,
                                          true, false>
    : static_graph_max_temporary_slots<StaticRegistry, RuntimeDependencies,
                                       true> {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct static_graph_temporary_slots_bound<StaticRegistry, RuntimeDependencies,
                                          true, true>
    : static_graph_total_temporary_slots<
          typename StaticRegistry::interface_bindings> {};

template <bool Resolvable, typename Visited, typename Order,
          bool ContainsCycle = false>
struct graph_visit_result {
    static constexpr bool resolvable = Resolvable;
    static constexpr bool contains_cycle = ContainsCycle;
    static constexpr bool acyclic = Resolvable && !ContainsCycle;
    using visited = Visited;
    using order = Order;
};

template <typename HeadResult, typename TailResult, bool Acyclic>
struct graph_visit_merge;

template <typename HeadResult, typename TailResult>
struct graph_visit_merge<HeadResult, TailResult, true> {
    using type = graph_visit_result<
        true, typename TailResult::visited,
        type_list_cat_t<typename HeadResult::order, typename TailResult::order>,
        HeadResult::contains_cycle || TailResult::contains_cycle>;
};

template <typename HeadResult, typename TailResult>
struct graph_visit_merge<HeadResult, TailResult, false> {
    using type = graph_visit_result<false, typename TailResult::visited, void,
                                    HeadResult::contains_cycle ||
                                        TailResult::contains_cycle>;
};

template <typename RecurseResult, typename Binding, bool Acyclic>
struct graph_visit_append;

template <typename RecurseResult, typename Binding>
struct graph_visit_append<RecurseResult, Binding, true> {
    using type = graph_visit_result<
        true,
        type_list_cat_t<typename RecurseResult::visited, type_list<Binding>>,
        type_list_cat_t<typename RecurseResult::order, type_list<Binding>>,
        RecurseResult::contains_cycle>;
};

template <typename RecurseResult, typename Binding>
struct graph_visit_append<RecurseResult, Binding, false> {
    using type = graph_visit_result<false, typename RecurseResult::visited,
                                    void, RecurseResult::contains_cycle>;
};

template <typename Bindings, typename StaticRegistry, typename Visiting,
          typename Visited, bool RuntimeDependencies>
struct graph_visit_all;

template <typename Binding, typename StaticRegistry, typename Visiting,
          typename Visited, bool RuntimeDependencies>
struct graph_visit_one;

template <typename StaticRegistry, typename Visiting, typename Visited,
          bool RuntimeDependencies>
struct graph_visit_all<type_list<>, StaticRegistry, Visiting, Visited,
                       RuntimeDependencies> {
    using type = graph_visit_result<true, Visited, type_list<>>;
};

template <typename StaticRegistry, typename Visiting, typename Visited,
          bool RuntimeDependencies>
struct graph_visit_all<void, StaticRegistry, Visiting, Visited,
                       RuntimeDependencies> {
    using type = graph_visit_result<true, Visited, type_list<>>;
};

template <typename Head, typename... Tail, typename StaticRegistry,
          typename Visiting, typename Visited, bool RuntimeDependencies>
struct graph_visit_all<type_list<Head, Tail...>, StaticRegistry, Visiting,
                       Visited, RuntimeDependencies> {
  private:
    using head_result =
        typename graph_visit_one<Head, StaticRegistry, Visiting, Visited,
                                 RuntimeDependencies>::type;

    using tail_result = std::conditional_t<
        head_result::resolvable,
        typename graph_visit_all<type_list<Tail...>, StaticRegistry, Visiting,
                                 typename head_result::visited,
                                 RuntimeDependencies>::type,
        graph_visit_result<false, typename head_result::visited, void>>;

  public:
    using type = typename graph_visit_merge<head_result, tail_result,
                                            head_result::resolvable &&
                                                tail_result::resolvable>::type;
};

template <typename Binding, typename StaticRegistry, typename Visiting,
          typename Visited, bool RuntimeDependencies, bool InVisiting,
          bool InVisited>
struct graph_visit_one_impl;

template <typename Binding, typename StaticRegistry, typename Visiting,
          typename Visited, bool RuntimeDependencies>
struct graph_visit_one_impl<Binding, StaticRegistry, Visiting, Visited,
                            RuntimeDependencies, true, false> {
    using type = std::conditional_t<
        static_cycle_uses_cyclical_storage_v<Binding, Visiting>,
        graph_visit_result<true, Visited, type_list<>, true>,
        graph_visit_result<false, Visited, void>>;
};

template <typename Binding, typename StaticRegistry, typename Visiting,
          typename Visited, bool RuntimeDependencies>
struct graph_visit_one_impl<Binding, StaticRegistry, Visiting, Visited,
                            RuntimeDependencies, false, true> {
    using type = graph_visit_result<true, Visited, type_list<>>;
};

template <typename Binding, typename StaticRegistry, typename Visiting,
          typename Visited, bool RuntimeDependencies>
struct graph_visit_one_impl<Binding, StaticRegistry, Visiting, Visited,
                            RuntimeDependencies, false, false> {
  private:
    using node =
        static_graph_node_t<Binding, StaticRegistry, RuntimeDependencies>;

    using recurse_result =
        typename graph_visit_all<typename node::dependency_bindings,
                                 StaticRegistry,
                                 type_list_cat_t<Visiting, type_list<Binding>>,
                                 Visited, RuntimeDependencies>::type;

  public:
    using type = typename graph_visit_append<recurse_result, Binding,
                                             recurse_result::resolvable>::type;
};

template <typename Binding, typename StaticRegistry, typename Visiting,
          typename Visited, bool RuntimeDependencies>
struct graph_visit_one {
    using type = typename graph_visit_one_impl<
        Binding, StaticRegistry, Visiting, Visited, RuntimeDependencies,
        type_list_contains_v<Binding, Visiting>,
        type_list_contains_v<Binding, Visited>>::type;
};

template <typename StaticRegistry, typename Visiting, typename Visited,
          bool RuntimeDependencies>
struct graph_visit_one<void, StaticRegistry, Visiting, Visited,
                       RuntimeDependencies> {
    using type = graph_visit_result<true, Visited, type_list<>>;
};

template <typename StaticRegistry, bool RuntimeDependencies = false>
struct basic_static_graph_topology_analysis {
  private:
    using traversal =
        typename graph_visit_all<typename StaticRegistry::interface_bindings,
                                 StaticRegistry, type_list<>, type_list<>,
                                 RuntimeDependencies>::type;

  public:
    static constexpr bool resolvable = traversal::resolvable;
    static constexpr bool contains_cycle = traversal::contains_cycle;
    static constexpr bool acyclic = traversal::acyclic;
    using topological_bindings =
        std::conditional_t<acyclic, typename traversal::order, void>;
};

template <typename StaticRegistry, bool RuntimeDependencies = false>
struct basic_static_execution_traits {
  private:
    using topology = basic_static_graph_topology_analysis<StaticRegistry,
                                                          RuntimeDependencies>;

  public:
    static constexpr bool resolvable = topology::resolvable;
    static constexpr bool contains_cycle = topology::contains_cycle;
    static constexpr bool acyclic = topology::acyclic;
    static constexpr std::size_t max_preserved_closure_depth =
        static_graph_preserved_closure_depth_bound<
            StaticRegistry, RuntimeDependencies, resolvable,
            contains_cycle>::value;
    static constexpr std::size_t max_destructible_slots =
        static_graph_destructible_slots_bound<StaticRegistry,
                                              RuntimeDependencies, resolvable,
                                              contains_cycle>::value;
    static constexpr std::size_t max_temporary_slots =
        static_graph_temporary_slots_bound<StaticRegistry, RuntimeDependencies,
                                           resolvable, contains_cycle>::value;
    static constexpr std::size_t max_temporary_size =
        static_graph_max_temporary_size<StaticRegistry, resolvable>::value;
    static constexpr std::size_t max_temporary_align =
        static_graph_max_temporary_align<StaticRegistry, resolvable>::value;
};

template <typename StaticRegistry, bool RuntimeDependencies = false>
using graph_analysis =
    basic_static_graph_topology_analysis<StaticRegistry, RuntimeDependencies>;

template <typename StaticRegistry, bool RuntimeDependencies = false>
using execution_traits =
    basic_static_execution_traits<StaticRegistry, RuntimeDependencies>;

template <typename StaticRegistry>
using static_execution_traits =
    basic_static_execution_traits<StaticRegistry, false>;



template <typename StaticSource, typename = void> struct static_graph;

template <typename StaticSource>
struct static_graph<StaticSource,
                    std::void_t<static_bindings_source_t<StaticSource>>>
    : static_graph<static_bindings_source_t<StaticSource>, void> {};

template <typename... Registrations>
struct static_graph<static_registry<Registrations...>, void>
    : private static_registry_dependency_diagnostics<
          typename static_registry<Registrations...>::interface_bindings,
          binding_model<Registrations>...> {
    using static_registry_type = static_registry<Registrations...>;
    using interface_bindings =
        typename static_registry_type::interface_bindings;
    using nodes =
        static_graph_nodes_t<interface_bindings, static_registry_type>;

    static_assert(static_registry_type::valid,
                  "static_graph requires a valid compile-time bindings source");

    static constexpr bool resolvable =
        graph_analysis<static_registry_type>::resolvable;
    static constexpr bool contains_cycle =
        graph_analysis<static_registry_type>::contains_cycle;
    static constexpr bool acyclic =
        graph_analysis<static_registry_type>::acyclic;
    using topological_bindings = typename graph_analysis<
        static_registry_type>::topological_bindings;
    using topological_nodes =
        static_graph_nodes_t<topological_bindings,
                                     static_registry_type>;

    template <typename Interface>
    using binding = typename static_registry_type::template binding<Interface>;

    template <typename Interface>
    using node =
        static_graph_node_t<binding<Interface>, static_registry_type>;

    template <typename Interface>
    using dependency_bindings =
        typename static_registry_type::template dependency_bindings<Interface>;

    template <typename Interface>
    using dependency_nodes =
        dependency_graph_nodes_t<dependency_bindings<Interface>,
                                         static_registry_type>;
};

} // export namespace silicon::di

// --- static/activation_set.h ---



#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

export namespace silicon::di {


template <bool RuntimeDependencies, typename... Registrations>
class basic_static_activation_set;

template <typename Registration> struct binding_storage_slot {
    using binding_model = binding_model<Registration>;
    using storage_type = typename binding_model::storage_type;

    storage_type storage;
};

template <typename BindingModel>
struct binding_factory_is_default_constructible
    : std::bool_constant<std::is_default_constructible_v<
          typename BindingModel::factory_type>> {};

template <typename BindingModel>
struct binding_storage_is_default_constructible
    : std::bool_constant<std::is_default_constructible_v<
          typename BindingModel::storage_type>> {};

struct no_dependency_context {
    template <typename T, typename Container> T resolve(Container&) = delete;
};

template <typename Factory>
inline constexpr bool factory_without_dependencies_v =
    std::is_same_v<typename factory_traits<Factory>::dependencies, type_list<>>;

template <typename Request, typename StorageType>
inline constexpr bool stored_request_identity_v = [] {
    using stored_type = std::remove_cv_t<
        std::remove_reference_t<typename StorageType::stored_type>>;
    return std::is_same_v<Request, stored_type> ||
           std::is_same_v<Request, stored_type&> ||
           std::is_same_v<Request, const stored_type&> ||
           std::is_same_v<Request, stored_type*> ||
           std::is_same_v<Request, const stored_type*>;
}();

template <typename BindingModel>
inline constexpr bool binding_has_conversion_cache_v = [] {
    using conversions_type = typename BindingModel::storage_type::conversions;
    return conversions_type::is_stable &&
           type_list_size_v<typename conversions_type::conversion_types> != 0;
}();

template <typename Closure, typename Registration> struct binding_closure_slot {
    Closure closure;
};

template <bool RuntimeDependencies, typename Registration,
          typename BindingsType =
              typename binding_model<Registration>::bindings_type>
struct local_binding_scope_slot {};

template <bool RuntimeDependencies, typename Registration,
          typename... LocalRegistrations>
struct local_binding_scope_slot<RuntimeDependencies, Registration,
                                static_registry<LocalRegistrations...>> {
    basic_static_activation_set<RuntimeDependencies, LocalRegistrations...>
        scope;
};

template <typename Registration, bool Enabled = binding_has_conversion_cache_v<
                                     binding_model<Registration>>>
struct binding_conversion_cache_slot;

template <typename Registration, bool Enabled>
struct registration_conversion_types {
    using binding_model = binding_model<Registration>;
    using storage_type = typename binding_model::storage_type;
    using conversions_type = typename storage_type::conversions;
    using type = typename conversions_type::conversion_types;
};

template <typename Registration, bool Enabled>
using registration_conversion_cache_base = binding_conversion_cache_base<
    Enabled,
    typename registration_conversion_types<Registration, Enabled>::type>;

template <typename Registration, bool Enabled>
struct binding_conversion_cache_slot
    : registration_conversion_cache_base<Registration, Enabled> {
    using binding_model = binding_model<Registration>;
    using conversions_type = typename binding_model::storage_type::conversions;
    using base_type = registration_conversion_cache_base<Registration, Enabled>;

    using base_type::construct_conversion;
};

template <bool RuntimeDependencies, typename... Registrations>
struct basic_static_activation_closure;

template <typename... Registrations>
struct basic_static_activation_closure<false, Registrations...> {
    using type = static_context_closure<
        basic_static_execution_traits<static_registry<Registrations...>,
                                              false>::max_destructible_slots,
        basic_static_execution_traits<static_registry<Registrations...>,
                                              false>::max_temporary_slots,
        basic_static_execution_traits<static_registry<Registrations...>,
                                              false>::max_temporary_size == 0
            ? 1
            : basic_static_execution_traits<
                  static_registry<Registrations...>, false>::max_temporary_size,
        basic_static_execution_traits<static_registry<Registrations...>,
                                              false>::max_temporary_align == 0
            ? alignof(std::max_align_t)
            : basic_static_execution_traits<
                  static_registry<Registrations...>,
                  false>::max_temporary_align>;
};

template <typename... Registrations>
struct basic_static_activation_closure<true, Registrations...> {
    using type = fixed_context_closure<
        basic_static_execution_traits<static_registry<Registrations...>,
                                              true>::max_destructible_slots,
        basic_static_execution_traits<static_registry<Registrations...>,
                                              true>::max_temporary_slots,
        basic_static_execution_traits<static_registry<Registrations...>,
                                              true>::max_temporary_size == 0
            ? 1
            : basic_static_execution_traits<
                  static_registry<Registrations...>, true>::max_temporary_size,
        basic_static_execution_traits<static_registry<Registrations...>,
                                              true>::max_temporary_align == 0
            ? alignof(std::max_align_t)
            : basic_static_execution_traits<
                  static_registry<Registrations...>,
                  true>::max_temporary_align>;
};

template <bool RuntimeDependencies, typename... Registrations>
using static_activation_closure_t =
    typename basic_static_activation_closure<RuntimeDependencies,
                                             Registrations...>::type;

template <bool RuntimeDependencies, typename... Registrations>
class static_binding_storage
    : private binding_conversion_cache_slot<Registrations>...,
      private binding_storage_slot<Registrations>...,
      private local_binding_scope_slot<RuntimeDependencies, Registrations>... {
    template <typename Registration>
    using storage_slot = binding_storage_slot<Registration>;

    template <typename Registration>
    using conversion_cache_slot = binding_conversion_cache_slot<Registration>;

    template <typename Registration>
    using local_scope_slot =
        local_binding_scope_slot<RuntimeDependencies, Registration>;

  public:
    template <typename Registration> auto& get_storage() {
        return static_cast<storage_slot<Registration>&>(*this).storage;
    }

    template <typename BindingModel> auto& get_storage_for_model() {
        return get_storage<typename BindingModel::registration_type>();
    }

    template <typename BindingModel> auto& get_conversion_cache_for_model() {
        return static_cast<
            conversion_cache_slot<typename BindingModel::registration_type>&>(
            *this);
    }

    template <typename BindingModel> auto& get_local_scope_for_model() {
        return static_cast<
                   local_scope_slot<typename BindingModel::registration_type>&>(
                   *this)
            .scope;
    }
};

template <typename State, typename Host, typename BindingModel>
struct binding_activation {
    State& state;
    Host& host;

    template <typename T, bool RemoveRvalueReferences, typename Context>
    decltype(auto) resolve(Context& context) {
        using local_bindings = typename BindingModel::bindings_type;
        if constexpr (std::is_void_v<local_bindings>) {
            return host.template resolve<T, RemoveRvalueReferences>(context);
        } else {
            return state.template resolve_local_binding<
                T, RemoveRvalueReferences, local_bindings, BindingModel>(
                host, context);
        }
    }

    template <typename T, bool RemoveRvalueReferences, typename Context,
              typename Key>
    decltype(auto) resolve(Context& context, key<Key>) {
        using local_bindings = typename BindingModel::bindings_type;
        if constexpr (std::is_void_v<local_bindings>) {
            return host.template resolve<T, RemoveRvalueReferences>(
                context, key<Key>{});
        } else {
            return state.template resolve_local_binding<
                T, RemoveRvalueReferences, local_bindings, BindingModel, Key>(
                host, context);
        }
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache,
              typename Context, typename Key>
    decltype(auto) resolve(Context& context, key<Key>) {
        (void)CheckCache;
        return resolve<T, RemoveRvalueReferences>(context, key<Key>{});
    }

    template <typename T, typename Context>
    decltype(auto) resolve(Context& context) {
        return state.template resolve_binding_type<T, BindingModel>(host,
                                                                    context);
    }

    template <typename T, typename Context, typename Source>
    decltype(auto) resolve_conversion(Context& context, Source&& source) {
        return state.template resolve_conversion<T, BindingModel>(
            context, std::forward<Source>(source));
    }
};

template <typename Request, typename StorageType> struct request_capabilities {
    using conversions = typename StorageType::conversions;
    using request_leaf_type =
        leaf_type_t<std::remove_cv_t<std::remove_reference_t<Request>>>;
    using base_types = std::conditional_t<
        std::is_pointer_v<Request>, typename conversions::pointer_types,
        std::conditional_t<
            std::is_lvalue_reference_v<Request>,
            typename conversions::lvalue_reference_types,
            std::conditional_t<std::is_rvalue_reference_v<Request>,
                               typename conversions::rvalue_reference_types,
                               typename conversions::value_types>>>;

    template <typename Type, typename Leaf,
              bool NeedsResolution =
                  std::is_same_v<leaf_type_t<Type>, runtime_type>>
    struct resolved_request_conversion_type {
        using type = Type;
    };

    template <typename Type, typename Leaf>
    struct resolved_request_conversion_type<Type, Leaf, true> {
        using type = resolved_type_t<Type, Leaf>;
    };

    template <typename Types, typename Leaf>
    struct resolved_request_conversion_list;

    template <typename Leaf, typename... Types>
    struct resolved_request_conversion_list<type_list<Types...>, Leaf> {
        using type = type_list<
            typename resolved_request_conversion_type<Types, Leaf>::type...>;
    };

    using type =
        typename resolved_request_conversion_list<base_types,
                                                  request_leaf_type>::type;
};

template <typename Request, typename CapabilityTypes>
struct request_capability_match;

template <typename Capability>
using unwrapped_static_capability_t =
    typename annotated_traits<keyed_type_t<Capability>>::type;

template <typename Request>
struct request_capability_match<Request, type_list<>> {
    using type = void;
};

template <typename Request, typename Head, typename... Tail>
struct request_capability_match<Request, type_list<Head, Tail...>> {
    using type = std::conditional_t<
        std::is_same_v<lookup_type_t<Head>, request_lookup_type_t<Request>> ||
            std::is_same_v<
                request_lookup_type_t<unwrapped_static_capability_t<Head>>,
                request_lookup_type_t<Request>>,
        Head,
        typename request_capability_match<Request, type_list<Tail...>>::type>;
};

template <typename Request, typename InterfaceBinding>
struct binding_supports_request {
    using capability_types = typename binding_request_types<InterfaceBinding>::type;
    using type =
        typename request_capability_match<Request, capability_types>::type;

    static constexpr bool value = !std::is_void_v<type>;
};

template <typename Request, typename InterfaceBinding>
inline constexpr bool binding_supports_request_v =
    binding_supports_request<Request, InterfaceBinding>::value;

template <typename State, typename Host, typename InterfaceBinding>
struct static_binding_resolver {
    using binding_model_type = typename InterfaceBinding::binding_model_type;
    using interface_type = typename InterfaceBinding::interface_type;
    using raw_interface_type = typename annotated_traits<interface_type>::type;
    using storage_type = typename binding_model_type::storage_type;

    State& state;
    Host& host;

    static constexpr type_descriptor registered_type() {
        return describe_type<typename storage_type::type>();
    }

    template <typename Request, typename Context>
    as_expected_t<Request> resolve(Context& context, instance_cache_sink cache = {}) {
        static_assert(State::runtime_dependencies ||
                          binding_supports_request_v<Request, InterfaceBinding>,
                      "static resolution cannot satisfy a request the storage "
                      "does not publish");
        using capability_types = typename binding_request_types<InterfaceBinding>::type;
        using capability =
            typename request_capability_match<Request, capability_types>::type;
        if constexpr (!std::is_void_v<capability> &&
                      !storage_type::conversions::is_stable &&
                      !std::is_reference_v<Request> &&
                      !std::is_pointer_v<Request>) {
            return consume_request<Request, capability>(
                context, [](auto&& instance) -> Request {
                    return std::forward<decltype(instance)>(instance);
                });
        }

        void* ptr = nullptr;
        if constexpr (!std::is_void_v<capability>) {
            // Stay on the normal materialization path even for identity
            // pointer/reference requests. Some storages publish the stored
            // instance through pointer-like source capabilities, so taking the
            // address of `storage.resolve(...)` would capture the address of a
            // stack-local pointer variable instead of the bound object.
            ptr = resolve_request_address<Request, capability>(context);
        } else {
            return std::unexpected(make_type_not_convertible_exception(
                describe_type<Request>(), registered_type(), context));
        }
        if constexpr (storage_type::conversions::is_stable) {
            cache(ptr);
        }
        return convert_resolved_binding<Request>(ptr);
    }

    template <typename Request, typename Context, typename Fn>
    as_expected_t<Request> consume(Context& context, Fn&& fn) {
        static_assert(State::runtime_dependencies ||
                          binding_supports_request_v<Request, InterfaceBinding>,
                      "static resolution cannot satisfy a request the storage "
                      "does not publish");
        using capability_types = typename binding_request_types<InterfaceBinding>::type;
        using capability =
            typename request_capability_match<Request, capability_types>::type;

        if constexpr (!std::is_void_v<capability>) {
            return consume_request<Request, capability>(context,
                                                        std::forward<Fn>(fn));
        } else {
            return std::unexpected(make_type_not_convertible_exception(
                describe_type<Request>(), registered_type(), context));
        }
    }

    template <typename Request, typename T, typename Context>
    void* resolve_request_address(Context& context) {
        using conversion_request_type =
            std::remove_reference_t<resolved_type_t<T, raw_interface_type>>;
        using target_type =
            std::remove_reference_t<resolved_type_t<T, raw_interface_type>>;
        constexpr bool uses_stored_request_identity =
            stored_request_identity_v<Request, storage_type>;

        binding_activation<State, Host, binding_model_type> activation{state,
                                                                       host};
        if constexpr (uses_stored_request_identity) {
            if constexpr (!State::runtime_dependencies) {
                return materialize_binding_source(
                    context,
                    state.template get_storage_for_model<binding_model_type>(),
                    activation, [&](auto&& source) -> void* {
                        auto&& instance =
                            resolve_binding_request<Request,
                                                            storage_type>(
                                activation, context,
                                std::forward<decltype(source)>(source));
                        return get_address_as<target_type>(
                            context,
                            std::forward<decltype(instance)>(instance));
                    });
            } else {
                return forward_binding_request<Request>(
                    context,
                    state.template get_storage_for_model<binding_model_type>(),
                    activation, activation, [&](auto&& instance) -> void* {
                        return get_address_as<target_type>(
                            context,
                            std::forward<decltype(instance)>(instance));
                    });
            }
        } else {
            return forward_binding_resolution_request<
                conversion_request_type>(
                context,
                state.template get_storage_for_model<binding_model_type>(),
                activation,
                state.template get_closure<
                    typename binding_model_type::registration_type>(),
                activation, [&](auto&& instance) -> void* {
                    return get_address_as<target_type>(
                        context, std::forward<decltype(instance)>(instance));
                });
        }
    }

    template <typename Request, typename T, typename Context, typename Fn>
    decltype(auto) consume_request(Context& context, Fn&& fn) {
        constexpr bool uses_stored_request_identity =
            stored_request_identity_v<Request, storage_type>;

        if constexpr (uses_stored_request_identity) {
            binding_activation<State, Host, binding_model_type> activation{
                state, host};
            if constexpr (!State::runtime_dependencies) {
                return materialize_binding_source(
                    context,
                    state.template get_storage_for_model<binding_model_type>(),
                    activation, [&](auto&& source) -> decltype(auto) {
                        auto&& instance =
                            resolve_binding_request<Request,
                                                            storage_type>(
                                activation, context,
                                std::forward<decltype(source)>(source));
                        return consume_resolved_binding<Request>(
                            std::forward<decltype(instance)>(instance),
                            std::forward<Fn>(fn));
                    });
            } else {
                return consume_binding_request<Request>(
                    context,
                    state.template get_storage_for_model<binding_model_type>(),
                    activation, activation, std::forward<Fn>(fn));
            }
        } else {
            binding_activation<State, Host, binding_model_type> activation{
                state, host};
            return consume_binding_resolution_request<Request>(
                context,
                state.template get_storage_for_model<binding_model_type>(),
                activation,
                state.template get_closure<
                    typename binding_model_type::registration_type>(),
                activation, std::forward<Fn>(fn));
        }
    }
};

template <typename T, typename BindingModel, typename State, typename Context,
          typename Source>
decltype(auto) evaluate_static_conversion(State& state, Context& context,
                                          Source&& source) {
    return resolve_binding_conversion<T>(
        state.template get_storage_for_model<BindingModel>(),
        state.template get_conversion_cache_for_model<BindingModel>(), context,
        std::forward<Source>(source));
}

template <typename T, typename BindingModel, typename State, typename Host,
          typename Context>
decltype(auto) evaluate_static_binding(State& state, Host& host,
                                       Context& context) {
    binding_activation<State, Host, BindingModel> activation{state, host};
    return materialize_tracked_binding_source(
        context, state.template get_storage_for_model<BindingModel>(),
        activation,
        [&](auto&& source) -> decltype(auto) {
            return resolve_binding_value<T>(
                activation, context, std::forward<decltype(source)>(source));
        });
}

template <typename InterfaceBinding, typename State, typename Host>
auto make_static_binding_resolver(State& state, Host& host) {
    return static_binding_resolver<State, Host, InterfaceBinding>{state, host};
}

template <typename T, typename Key, typename StaticRegistryType, typename State,
          typename Host, typename Fn, typename Context>
std::size_t append_static_collection_impl(State& state, T& results, Host& host,
                                          Context& context, Fn&& fn) {
    using collection_type = collection_traits<T>;
    using resolve_type = typename collection_type::resolve_type;
    using interface_bindings = typename StaticRegistryType::template bindings<
        normalized_type_t<resolve_type>, Key>;

    static_assert(collection_type::is_collection,
                  "missing collection_traits specialization for type T");

    constexpr std::size_t count = type_list_size_v<interface_bindings>;
    if constexpr (count != 0) {
        silicon::di::for_each(interface_bindings{}, [&](auto binding_iterator) {
            using binding = typename decltype(binding_iterator)::type;
            auto resolver =
                make_static_binding_resolver<binding>(state, host);
            if constexpr (is_copy_constructible_v<resolve_type> &&
                          !std::is_reference_v<resolve_type>) {
                resolver.template consume<resolve_type>(
                    context, [&](auto&& value) {
                        fn(results, std::forward<decltype(value)>(value));
                    });
            } else {
                fn(results,
                   resolver.template resolve<resolve_type>(context));
            }
        });
    }

    return count;
}

template <typename T, typename Key, typename StaticRegistryType, typename State,
          typename Host, typename Fn, typename Context>
T construct_static_collection_impl(State& state, Host& host, Context& context,
                                   Fn&& fn) {
    using collection_type = collection_traits<T>;
    using resolve_type = typename collection_type::resolve_type;
    using interface_bindings = typename StaticRegistryType::template bindings<
        normalized_type_t<resolve_type>, Key>;

    static_assert(collection_type::is_collection,
                  "missing collection_traits specialization for type T");
    static_assert(
        type_list_size_v<interface_bindings> != 0,
        "static container cannot construct a collection for an unbound type");

    T results;
    collection_type::reserve(results, type_list_size_v<interface_bindings>);
    append_static_collection_impl<T, Key, StaticRegistryType>(
        state, results, host, context, std::forward<Fn>(fn));
    return results;
}

template <typename T, typename Key, typename StaticRegistryType, typename State,
          typename Host, typename Context>
T construct_static_collection_default_impl(State& state, Host& host,
                                           Context& context) {
    using collection_type = collection_traits<T>;
    using resolve_type = typename collection_type::resolve_type;
    using interface_bindings = typename StaticRegistryType::template bindings<
        normalized_type_t<resolve_type>, Key>;

    static_assert(collection_type::is_collection,
                  "missing collection_traits specialization for type T");
    static_assert(
        type_list_size_v<interface_bindings> != 0,
        "static container cannot construct a collection for an unbound type");

    if constexpr (collection_type::has_fixed_size_construct) {
        T results = collection_type::make_fixed_size(
            type_list_size_v<interface_bindings>);
        std::size_t index = 0;
        append_static_collection_impl<T, Key, StaticRegistryType>(
            state, results, host, context, [&](auto&, auto&& value) {
                collection_type::set(results, index,
                                     std::forward<decltype(value)>(value));
                ++index;
            });
        return results;
    } else {
        return construct_static_collection_impl<T, Key, StaticRegistryType>(
            state, host, context, [](auto& collection, auto&& value) {
                collection_type::add(collection,
                                     std::forward<decltype(value)>(value));
            });
    }
}

template <typename Derived, bool RuntimeDependencies, typename... Registrations>
class basic_static_activation_set_base
    : private binding_closure_slot<
          static_activation_closure_t<RuntimeDependencies, Registrations...>,
          Registrations>... {
    template <typename Registration>
    using closure_holder = binding_closure_slot<
        static_activation_closure_t<RuntimeDependencies, Registrations...>,
        Registration>;

  public:
    static constexpr bool runtime_dependencies = RuntimeDependencies;

    using rtti_type = rtti<static_provider>;

    template <typename Registration> auto& get_closure() {
        return static_cast<closure_holder<Registration>&>(*this).closure;
    }

    template <typename T, typename BindingModel, typename Context,
              typename Source>
    decltype(auto) resolve_conversion(Context& context, Source&& source) {
        return evaluate_static_conversion<T, BindingModel>(
            derived(), context, std::forward<Source>(source));
    }

    template <typename T, typename BindingModel, typename Host,
              typename Context>
    decltype(auto) resolve_binding_type(Host& host, Context& context) {
        return evaluate_static_binding<T, BindingModel>(derived(), host,
                                                                context);
    }

    template <typename InterfaceBinding, typename Host>
    auto make_binding_resolver(Host& host) {
        return make_static_binding_resolver<InterfaceBinding>(
            derived(), host);
    }

    template <typename T, bool RemoveRvalueReferences, typename LocalRegistry,
              typename BindingModel, typename Key = void, typename Host,
              typename Context,
              typename R = resolve_result_t<T, RemoveRvalueReferences>>
    as_expected_t<R> resolve_local_binding(Host& host, Context& context) {
        if constexpr (collection_traits<R>::is_collection) {
            using collection_type = collection_traits<R>;
            R results;
            auto append = [](auto& values, auto&& value) {
                collection_type::add(values,
                                     std::forward<decltype(value)>(value));
            };
            auto& local_scope =
                derived().template get_local_scope_for_model<BindingModel>();
            const auto local_count =
                local_scope.template append_static_collection<R, Key,
                                                              LocalRegistry>(
                    results, host, context, append);
            const auto host_count =
                host.template append_collection<R, Key>(results, context,
                                                        append);
            if (local_count + host_count == 0) {
                return std::unexpected(
                    make_collection_type_not_found_exception<
                        R, typename collection_type::resolve_type>());
            }
            return results;
        } else {
            using selection = static_binding_t<
                typename LocalRegistry::template bindings<R, Key>>;
            if constexpr (selection::status ==
                          binding_selection_status::kFound) {
                using binding = typename selection::binding_type;
                auto& local_scope =
                    derived()
                        .template get_local_scope_for_model<BindingModel>();
                auto resolver =
                    local_scope.template make_binding_resolver<binding>(host);
                return resolver.template resolve<R>(context);
            } else {
                if constexpr (std::is_void_v<Key>) {
                    return host.template resolve<T, RemoveRvalueReferences>(
                        context);
                } else {
                    return host.template resolve<T, RemoveRvalueReferences>(
                        context, key<Key>{});
                }
            }
        }
    }

    template <typename T, typename Key, typename StaticRegistryType,
              typename Host, typename Fn, typename Context>
    std::size_t append_static_collection(T& results, Host& host,
                                         Context& context, Fn&& fn) {
        return append_static_collection_impl<T, Key,
                                                     StaticRegistryType>(
            derived(), results, host, context, std::forward<Fn>(fn));
    }

    template <typename T, typename Key, typename StaticRegistryType,
              typename Host, typename Fn, typename Context>
    T construct_static_collection(Host& host, Context& context, Fn&& fn) {
        return construct_static_collection_impl<T, Key,
                                                        StaticRegistryType>(
            derived(), host, context, std::forward<Fn>(fn));
    }

    template <typename T, typename Key, typename StaticRegistryType,
              typename Host, typename Context>
    T construct_static_collection(Host& host, Context& context) {
        return construct_static_collection_default_impl<
            T, Key, StaticRegistryType>(derived(), host, context);
    }

  private:
    Derived& derived() { return static_cast<Derived&>(*this); }
};

template <bool RuntimeDependencies, typename... Registrations>
class basic_static_activation_set
    : public basic_static_activation_set_base<
          basic_static_activation_set<RuntimeDependencies, Registrations...>,
          RuntimeDependencies, Registrations...>,
      private static_binding_storage<RuntimeDependencies, Registrations...> {
  public:
    using static_binding_storage<
        RuntimeDependencies,
        Registrations...>::get_conversion_cache_for_model;
    using static_binding_storage<RuntimeDependencies,
                                 Registrations...>::get_local_scope_for_model;
    using static_binding_storage<RuntimeDependencies,
                                 Registrations...>::get_storage;
    using static_binding_storage<RuntimeDependencies,
                                 Registrations...>::get_storage_for_model;
};

template <typename... Registrations>
using static_binding_scope =
    basic_static_activation_set<false, Registrations...>;

template <typename... Registrations>
using binding_scope = basic_static_activation_set<true, Registrations...>;

template <typename... Registrations>
using static_storage_state = static_binding_storage<false, Registrations...>;

template <bool RuntimeDependencies, typename StorageState,
          typename... Registrations>
class basic_static_activation_set_ref
    : public basic_static_activation_set_base<
          basic_static_activation_set_ref<RuntimeDependencies, StorageState,
                                          Registrations...>,
          RuntimeDependencies, Registrations...> {
  public:
    explicit basic_static_activation_set_ref(StorageState& state)
        : state_(&state) {}

    template <typename Registration> auto& get_storage() {
        return state_->template get_storage<Registration>();
    }

    template <typename BindingModel> auto& get_storage_for_model() {
        return state_->template get_storage_for_model<BindingModel>();
    }

    template <typename BindingModel> auto& get_conversion_cache_for_model() {
        return state_->template get_conversion_cache_for_model<BindingModel>();
    }

    template <typename BindingModel> auto& get_local_scope_for_model() {
        return state_->template get_local_scope_for_model<BindingModel>();
    }

  private:
    StorageState* state_;
};

template <typename StorageState, typename... Registrations>
using static_binding_scope_ref =
    basic_static_activation_set_ref<false, StorageState, Registrations...>;

template <typename StorageState, typename... Registrations>
using binding_scope_ref =
    basic_static_activation_set_ref<true, StorageState, Registrations...>;


} // export namespace silicon::di

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// --- static/local_resolution.h ---



export namespace silicon::di {


template <typename Host, typename StaticRegistry> class binding_resolution;

template <typename Host, typename... Registrations>
class binding_resolution<Host, static_registry<Registrations...>>
    : private binding_scope<Registrations...> {
    using static_registry_type = static_registry<Registrations...>;
    using self_type = binding_resolution<Host, static_registry_type>;
    using state_type = binding_scope<Registrations...>;

    template <typename T, typename Key>
    using binding_t = static_binding_t<
        typename static_registry_type::template bindings<T, Key>>;

    template <typename T, bool RemoveRvalueReferences, typename Key,
              typename Request>
    struct local_binding_source {
        using binding = binding_t<Request, Key>;
        static constexpr bool can_resolve =
            binding::status == binding_selection_status::kFound;

        self_type& self;

        constexpr binding_selection_status status() const {
            return binding::status;
        }

        template <typename ResolveRequest>
        decltype(auto) resolve(runtime_context& context) {
            return self.template resolve_binding<Request, Key>(context);
        }
    };

    template <typename T, bool RemoveRvalueReferences, typename Key,
              typename Request>
    struct host_binding_source {
        static constexpr bool can_resolve = true;

        Host& host;

        binding_selection_status status() const {
            return host.template binding_status<Request, Key>();
        }

        template <typename ResolveRequest>
        decltype(auto) resolve(runtime_context& context) {
            return host.template resolve_request<T, RemoveRvalueReferences, Key>(
                context);
        }
    };

    template <typename Request, typename Key>
    typename annotated_traits<Request>::type
    resolve_binding(runtime_context& context) {
        using selection = binding_t<Request, Key>;
        using binding = typename selection::binding_type;
        auto resolver = this->template make_binding_resolver<binding>(*this);
        return resolver.template resolve<Request>(context);
    }

    template <typename T, typename Key, typename Fn>
    T construct_collection(runtime_context& context, Fn&& fn) {
        using collection_type = collection_traits<T>;
        using resolve_type = typename collection_type::resolve_type;

        constexpr std::size_t static_count =
            type_list_size_v<typename static_registry_type::template bindings<
                normalized_type_t<resolve_type>, Key>>;
        return construct_binding_collection<T>(
            [&] { return host_->template count_collection<T, Key>(); },
            [&] { return static_count; },
            [&](auto& results, auto&& append) {
                host_->template append_collection<T, Key>(
                    results, context, std::forward<decltype(append)>(append));
            },
            [&](auto& results, auto&& append) {
                this->template append_static_collection<T, Key,
                                                        static_registry_type>(
                    results, *this, context,
                    std::forward<decltype(append)>(append));
            },
            std::forward<Fn>(fn));
    }

  public:
    using allocator_type = typename Host::allocator_type;

    static_assert(static_registry_type::valid,
                  "register_type bindings<...> requires a valid compile-time "
                  "bindings source");
    static_assert(graph_analysis<static_registry_type, true>::resolvable,
                  "register_type bindings<...> requires a resolvable "
                  "compile-time binding graph");
    static_assert((binding_factory_is_default_constructible<
                       binding_model<Registrations>>::value &&
                   ...),
                  "register_type bindings<...> requires default-constructible "
                  "local factories");
    static_assert((binding_storage_is_default_constructible<
                       binding_model<Registrations>>::value &&
                   ...),
                  "register_type bindings<...> requires default-constructible "
                  "local storage objects");

    template <typename Allocator>
    binding_resolution(Host* host, Allocator&&) : host_(host) {}

    allocator_type& get_allocator() { return host_->get_allocator(); }

    template <typename T, bool RemoveRvalueReferences, typename Key = void,
              typename R = resolve_result_t<T, RemoveRvalueReferences>>
    R resolve(runtime_context& context) {
        if constexpr (collection_traits<R>::is_collection) {
            return construct_collection<R, Key>(
                context, binding_collection_append{});
        } else {
            using request_type = R;
            local_binding_source<T, RemoveRvalueReferences, Key, request_type>
                local{*this};
            host_binding_source<T, RemoveRvalueReferences, Key, request_type>
                host{*host_};
            auto sources = make_two_binding_sources(
                local, host, host,
                binding_resolution_policy::kPreferPrimary);
            return resolve_from_binding_sources<T, request_type>(
                context, sources);
        }
    }

    template <typename T, bool RemoveRvalueReferences, typename Key,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve(runtime_context& context, key<Key>) {
        return resolve<T, RemoveRvalueReferences, Key>(context);
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache,
              typename Key,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve(runtime_context& context, key<Key>) {
        (void)CheckCache;
        return resolve<T, RemoveRvalueReferences, Key>(context);
    }

  private:
    Host* host_;
};


} // export namespace silicon::di


// ==============================================================================
// ==  runtime  —  runtime container traits & registry
// ==============================================================================

// --- runtime/registry.h ---



export namespace silicon::di {


template <typename StaticRegistry, typename ParentContainer>
class container_with_static_bindings;



template <typename ContainerTraits, typename Allocator, typename ParentRegistry,
          typename ResolveRoot>
class runtime_registry : public allocator_base<Allocator> {
    friend class runtime_context;
    template <typename, typename> friend class binding_resolution;
    template <typename, typename>
    friend class container_with_static_bindings;
    template <typename, typename, typename> friend class runtime_container;
    template <typename ContainerTraitsT, typename AllocatorT,
              typename ParentRegistryT, typename ResolveRootT>
    friend class runtime_registry;

    template <typename ContainerTraitsT, typename AllocatorT,
              typename ParentRegistryT, typename ResolveRootT>
    using rebind_t = runtime_registry<ContainerTraitsT, AllocatorT,
                                      ParentRegistryT, ResolveRootT>;
    using registry_type = runtime_registry<ContainerTraits, Allocator,
                                           ParentRegistry, ResolveRoot>;
    using resolve_root_type =
        std::conditional_t<std::is_same_v<void, ResolveRoot>, registry_type,
                           ResolveRoot>;
    using container_type = resolve_root_type;
    template <typename Registration>
    using registration_container_type =
        runtime_registry<typename ContainerTraits::template rebind_t<
                             type_list<typename ContainerTraits::tag_type,
                                       typename Registration::interface_type>>,
                         Allocator, resolve_root_type, resolve_root_type>;
    using parent_registry_type =
        std::conditional_t<std::is_same_v<void, ParentRegistry>, registry_type,
                           ParentRegistry>;

    static constexpr bool cache_enabled = ContainerTraits::cache_enabled;

  public:
    using container_traits_type = ContainerTraits;
    using allocator_type = Allocator;
    using rtti_type = typename ContainerTraits::rtti_type;
    using index_definition_type =
        typename ContainerTraits::index_definition_type;
    runtime_registry()
        : allocator_base<allocator_type>(allocator_type()) {}

    runtime_registry(allocator_type alloc)
        : allocator_base<allocator_type>(alloc) {}

    runtime_registry(resolve_root_type* root,
                     allocator_type alloc = allocator_type())
        : allocator_base<allocator_type>(alloc), resolve_root_(root) {

        if constexpr (!std::is_same_v<void, ParentRegistry>) {
            static_assert(
                !is_tagged_container_v<container_traits_type> ||
                    !std::is_same_v<typename container_traits_type::tag_type,
                                    typename parent_registry_type::
                                        container_traits_type::tag_type>,
                "static typemap based containers require parent and child "
                "container tags to be different");
        }
    }

    runtime_registry(const runtime_registry&) = delete;
    runtime_registry& operator=(const runtime_registry&) = delete;

    ~runtime_registry() { destroy_runtime_bindings(); }

    allocator_type& get_allocator() {
        return allocator_base<allocator_type>::get_allocator();
    }

  protected:
    bool has_runtime_registrations() const { return runtime_bindings_present_; }
    struct runtime_bindings_state;

    runtime_bindings_state* runtime_bindings_if_present();

    const runtime_bindings_state* runtime_bindings_if_present() const;

    runtime_bindings_state& ensure_runtime_bindings();

    void destroy_runtime_bindings();

    resolve_root_type* resolve_root() {
        if constexpr (std::is_same_v<void, ResolveRoot> ||
                      std::is_same_v<resolve_root_type, registry_type>) {
            return this;
        } else if constexpr (std::is_base_of_v<registry_type,
                                               resolve_root_type>) {
            return static_cast<resolve_root_type*>(this);
        } else {
            return resolve_root_;
        }
    }

    const resolve_root_type* resolve_root() const {
        if constexpr (std::is_same_v<void, ResolveRoot> ||
                      std::is_same_v<resolve_root_type, registry_type>) {
            return this;
        } else if constexpr (std::is_base_of_v<registry_type,
                                               resolve_root_type>) {
            return static_cast<const resolve_root_type*>(this);
        } else {
            return resolve_root_;
        }
    }

  public:
    template <typename... TypeArgs> auto& register_type() {
        return register_type_impl<TypeArgs...>(resolve_root(), none_t{},
                                               none_t{});
    }

    template <typename... TypeArgs, typename Arg>
    auto& register_type(Arg&& arg) {
        return register_type_impl<TypeArgs...>(
            resolve_root(), std::forward<Arg>(arg), none_t{});
    }

    template <typename... TypeArgs, typename IdType>
    auto& register_indexed_type(IdType&& id) {
        return register_type_impl<TypeArgs...>(
            resolve_root(), none_t{}, std::forward<IdType>(id));
    }

    template <typename... TypeArgs, typename Arg, typename IdType>
    auto& register_indexed_type(Arg&& arg, IdType&& id) {
        return register_type_impl<TypeArgs...>(
            resolve_root(), std::forward<Arg>(arg),
            std::forward<IdType>(id));
    }

    template <typename... TypeArgs, typename Fn>
    auto& register_type_collection(Fn&& fn) {
        using registration = type_registration<TypeArgs...>;
        return register_type<TypeArgs...>(callable(
            [this, collection_fn = std::forward<Fn>(fn)]() mutable {
                return this->template construct_collection<
                    typename registration::storage_type::type>(collection_fn);
            }));
    }

    template <typename... TypeArgs> auto& register_type_collection() {
        return register_type_collection<TypeArgs...>(
            binding_collection_append{});
    }

    template <typename... TypeArgs, typename Parent, typename Arg,
              typename IdType>
    auto& emplace_type_binding(Parent& parent, Arg&& arg, IdType&& id) {
        return register_type_impl<TypeArgs...>(
            &parent, std::forward<Arg>(arg), std::forward<IdType>(id));
    }

  protected:
    template <typename T, typename IdType = none_t,
              typename R = request_result_t<T>>
    R resolve(IdType&& id = IdType()) {
        return resolve_runtime_request<T>(std::forward<IdType>(id));
    }

    template <typename T, typename Factory = constructor<normalized_type_t<T>>,
              typename R = request_result_t<T>>
    R construct(Factory factory = Factory()) {
        return construct_runtime_request<T>(std::move(factory));
    }

    template <typename T> T construct_collection() {
        return construct_collection_runtime_request<T>();
    }

    template <typename T, typename Fn> T construct_collection(Fn&& fn) {
        return construct_collection_runtime_request<T>(std::forward<Fn>(fn));
    }

    template <typename T, typename Key> T construct_collection(key<Key>) {
        return construct_collection_runtime_request<T>(key<Key>{});
    }

    template <typename T, typename Fn, typename Key>
    T construct_collection(Fn&& fn, key<Key>) {
        return construct_collection_runtime_request<T>(
            std::forward<Fn>(fn), key<Key>{});
    }

    template <typename Signature = void, typename Callable>
    auto invoke(Callable&& callable) {
        return invoke_runtime_request<Signature>(
            std::forward<Callable>(callable));
    }

    template <typename T, typename IdType = none_t,
              typename R = request_result_t<T>>
    R resolve_runtime_request(IdType&& id = IdType()) {
        if constexpr (is_typed_key_v<IdType> &&
                      collection_traits<R>::is_collection) {
            return construct_collection_runtime_request<R>(
                std::decay_t<IdType>{});
        } else {
            if constexpr (cache_enabled) {
                if constexpr (is_none_v<std::decay_t<IdType>>) {
                    if (auto* state = runtime_bindings_if_present()) {
                        void* cache = state->type_cache.template get<T>();
                        if (cache) {
                            return convert_resolved_binding<
                                request_interface_t<T>>(cache);
                        }
                    }
                } else {
                    if (auto* state = runtime_bindings_if_present()) {
                        auto data = state->type_bindings.template get<
                            normalized_type_t<T>>();
                        if (data) {
                            if constexpr (is_typed_key_v<IdType>) {
                                registered_binding_entry* candidate = nullptr;
                                for (auto&& p : data->bindings) {
                                    auto& entry = p.second;
                                    if (!entry.key_type ||
                                        !(*entry.key_type ==
                                          rtti_type::template get_type_index<
                                              std::decay_t<IdType>>())) {
                                        continue;
                                    }

                                    if (candidate) {
                                        candidate = nullptr;
                                        break;
                                    }
                                    candidate = &entry;
                                }

                                if (candidate && candidate->cache) {
                                    return convert_resolved_binding<
                                        request_interface_t<T>>(
                                        candidate->cache);
                                }
                            } else {
                                auto indexed =
                                    data->template get_index<IdType>(
                                            get_allocator())
                                        .find(id);

                                if (indexed) {
                                    if (indexed->cache) {
                                        return convert_resolved_binding<
                                            request_interface_t<T>>(
                                            indexed->cache);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            runtime_context context;
            return resolve_impl<T, true, false, false>(
                context, std::forward<IdType>(id));
        }
    }

    template <typename T, typename Factory = constructor<normalized_type_t<T>>,
              typename R = request_result_t<T>>
    R construct_runtime_request(Factory factory = Factory()) {
        runtime_context context;
        if constexpr (std::is_same_v<Factory,
                                     constructor<normalized_type_t<T>>>) {
            if (binding_status<T>() !=
                binding_selection_status::kNotFound) {
                if constexpr (::silicon::di::
                                  rvalue_request_requires_explicit_conversion_v<
                                      T>) {
                    return resolve<T, false>(context, none_t{});
                } else if constexpr (construct_normalized_request_v<T>) {
                    return ::silicon::di::construct_request_or_wrap_normalized<T>(
                        [&]() { return resolve<T, false>(context, none_t{}); },
                        [&]() {
                            return resolve<normalized_type_t<T>, false>(
                                context, none_t{});
                        });
                } else {
                    return resolve<T, false>(context, none_t{});
                }
            } else if (binding_status<normalized_type_t<T>>() !=
                       binding_selection_status::kNotFound) {
                if constexpr (::silicon::di::
                                  rvalue_request_requires_explicit_conversion_v<
                                      T>) {
                    ::silicon::di::terminate_missing_rvalue_conversion<T>(true, context);
                } else if constexpr (construct_normalized_request_v<T>) {
                    return type_traits<std::decay_t<T>>::make(
                        resolve<normalized_type_t<T>, false>(context,
                                                             none_t{}));
                } else {
                    return resolve<T, false>(context, none_t{});
                }
            }
        }

        if constexpr (construct_factory_request_v<T>) {
            return factory.template construct<R>(context, *resolve_root());
        } else if constexpr (::silicon::di::
                                 rvalue_request_requires_explicit_conversion_v<
                                     T>) {
            ::silicon::di::terminate_missing_rvalue_conversion<T>(false, context);
        } else {
            return resolve<T, false>(context, none_t{});
        }
    }

    template <typename T> T construct_collection_runtime_request() {
        return construct_collection_runtime_request<T>(
            binding_collection_append{});
    }

    template <typename T, typename Fn>
    T construct_collection_runtime_request(Fn&& fn) {
        return construct_collection_runtime_request<T>(std::forward<Fn>(fn),
                                                       none_t{});
    }

    template <typename T, typename Key>
    T construct_collection_runtime_request(key<Key>) {
        return construct_collection_runtime_request<T>(
            binding_collection_append{}, key<Key>{});
    }

    template <typename T, typename Fn, typename Key>
    T construct_collection_runtime_request(Fn&& fn, key<Key>) {
        return construct_collection_runtime_request_impl<T, Key>(
            std::forward<Fn>(fn));
    }

    template <typename T, typename Fn>
    T construct_collection_runtime_request(Fn&& fn, none_t) {
        return construct_collection_runtime_request_impl<T, void>(
            std::forward<Fn>(fn));
    }

    template <typename T, typename Key, typename Fn>
    as_expected_t<T> construct_collection_runtime_request_impl(Fn&& fn) {
        using collection_type = collection_traits<T>;
        using resolve_type = typename collection_type::resolve_type;

        static_assert(collection_type::is_collection,
                      "missing collection_traits specialization for type T");

        T results;
        runtime_context context;
        const std::size_t count =
            count_runtime_collection<T>(collection_key<Key>());
        if (count == 0) {
            return std::unexpected(
                make_collection_type_not_found_exception<T, resolve_type>());
        }

        collection_type::reserve(results, count);
        append_runtime_collection(results, context, std::forward<Fn>(fn),
                                  collection_key<Key>());
        return results;
    }

    template <typename Signature = void, typename Callable>
    auto invoke_runtime_request(Callable&& callable) {
        using callable_type =
            std::remove_cv_t<std::remove_reference_t<Callable>>;
        using dispatch_signature =
            callable_dispatch_signature_t<Signature, callable_type>;

        runtime_context context;
        auto type_guard = context.template track_type<callable_type>();
        return callable_invoke<dispatch_signature>::construct(
            std::forward<Callable>(callable), context, *resolve_root());
    }

    template <typename Request, typename Key = void>
    binding_selection_status binding_status() {
        return binding_status_for_id<Request>(collection_key<Key>());
    }

    template <typename Request, typename IdType>
    binding_selection_status binding_status_for_id(IdType&& id) {
        using exact_type =
            std::remove_cv_t<std::remove_reference_t<request_interface_t<Request>>>;
        using lookup_type = normalized_type_t<Request>;
        auto* state = runtime_bindings_if_present();
        auto* data =
            state ? state->type_bindings.template get<exact_type>() : nullptr;
        if constexpr (!std::is_same_v<exact_type, lookup_type>) {
            if (!data && state) {
                data = state->type_bindings.template get<lookup_type>();
            }
        }
        auto selection = select_runtime_binding(data, std::forward<IdType>(id));
        return selection.status;
    }

    template <typename T, typename Key = void, typename Fn>
    std::size_t append_collection(T& results, runtime_context& context,
                                  Fn&& fn) {
        return append_runtime_collection(results, context, std::forward<Fn>(fn),
                                         collection_key<Key>());
    }

    template <typename T, typename Key = void> std::size_t count_collection() {
        return count_runtime_collection<T>(collection_key<Key>());
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve_request(runtime_context& context) {
        return resolve<T, RemoveRvalueReferences>(context,
                                                  collection_key<Key>());
    }

    template <typename CachedT>
    static void store_type_cache(void* context, void* ptr) {
        static_cast<registry_type*>(context)
            ->ensure_runtime_bindings()
            .type_cache.template insert<CachedT>(ptr);
    }

    static void store_index_cache(void* context, void* ptr) {
        static_cast<binding_cache_state*>(context)->cache = ptr;
    }

    template <typename T> static T& invalid_registration_return();

    struct binding_cache_state {
        void* cache = nullptr;
    };

    struct index_data : binding_cache_state {
        runtime_binding_interface<container_type>* binding = nullptr;

        index_data() = default;

        explicit index_data(
            runtime_binding_interface<container_type>* binding_ptr)
            : binding(binding_ptr) {}

        operator bool() const { return binding != nullptr; }
    };

    struct registered_binding_entry : binding_cache_state {
        runtime_binding_ptr<runtime_binding_interface<container_type>> binding;
        std::optional<typename rtti_type::type_index> key_type;

        registered_binding_entry(
            runtime_binding_ptr<runtime_binding_interface<container_type>>&&
                binding_ptr,
            std::optional<typename rtti_type::type_index> resolved_key_type =
                std::nullopt)
            : binding(std::move(binding_ptr)),
              key_type(std::move(resolved_key_type)) {}

        registered_binding_entry(const registered_binding_entry&) = delete;
        registered_binding_entry&
        operator=(const registered_binding_entry&) = delete;

        registered_binding_entry(registered_binding_entry&& other) noexcept
            : binding_cache_state{other.cache},
              binding(std::move(other.binding)),
              key_type(std::move(other.key_type)) {
            other.cache = nullptr;
        }

        registered_binding_entry&
        operator=(registered_binding_entry&& other) noexcept {
            if (this != &other) {
                this->cache = other.cache;
                other.cache = nullptr;
                binding = std::move(other.binding);
                key_type = std::move(other.key_type);
            }
            return *this;
        }
    };

    using index_definition_list_type = to_type_list_t<index_definition_type>;
    using index_type = index_impl<index_definition_list_type,
                                          index_data, allocator_type>;

    struct runtime_type_bindings : index_type {
        runtime_type_bindings(allocator_type& allocator)
            : index_type(allocator), bindings(allocator) {}

        typename ContainerTraits::template type_map_type<
            registered_binding_entry, allocator_type>
            bindings;
    };

    struct runtime_bindings_state {
        explicit runtime_bindings_state(allocator_type& allocator)
            : type_bindings(allocator), type_cache(allocator) {}

        typename ContainerTraits::template type_map_type<runtime_type_bindings,
                                                         allocator_type>
            type_bindings;
        typename ContainerTraits::template type_cache_type<void*,
                                                           allocator_type>
            type_cache;
    };

    using runtime_binding_interface_type =
        runtime_binding_interface<container_type>;
    using runtime_selection =
        runtime_binding_selection<runtime_binding_interface_type,
                                          binding_cache_state*>;
    template <typename Key>
    using collection_key_t =
        std::conditional_t<std::is_void_v<Key>, none_t, key<Key>>;

  protected:
    template <typename Key> static collection_key_t<Key> collection_key() {
        return {};
    }

    template <typename T, bool CheckCache, typename IdType>
    struct selected_runtime_binding {
        registry_type& registry;
        IdType& id;

        decltype(auto) select() {
            return registry.template runtime_source_select<T>(
                std::forward<IdType>(id));
        }

        template <typename Request, typename Selection>
        decltype(auto) resolve(runtime_context& context, Selection selection) {
            return registry.template runtime_source_resolve<T, CheckCache>(
                selection, context, std::forward<IdType>(id));
        }
    };

    template <typename T, bool RemoveRvalueReferences, bool MayAutoConstruct,
              typename IdType>
    struct missing_runtime_binding {
        registry_type& registry;
        IdType& id;

        template <typename Request>
        request_interface_t<Request> resolve(runtime_context& context) {
            return registry.template runtime_source_missing<
                T, RemoveRvalueReferences, MayAutoConstruct, IdType,
                request_interface_t<Request>>(context,
                                              std::forward<IdType>(id));
        }
    };

    template <typename Request, typename IdType = none_t>
    runtime_selection runtime_source_select(IdType&& id = IdType()) {
        using exact_type =
            std::remove_cv_t<std::remove_reference_t<request_interface_t<Request>>>;
        using lookup_type = normalized_type_t<Request>;
        auto* state = runtime_bindings_if_present();
        auto* data =
            state ? state->type_bindings.template get<exact_type>() : nullptr;
        if constexpr (!std::is_same_v<exact_type, lookup_type>) {
            if (!data && state) {
                data = state->type_bindings.template get<lookup_type>();
            }
        }
        return select_runtime_binding(data, std::forward<IdType>(id));
    }

    template <typename T, bool CheckCache, typename IdType>
    request_interface_t<T> runtime_source_resolve(runtime_selection selection,
                                runtime_context& context, IdType&& id) {
        (void)id;
        if constexpr (is_none_v<std::decay_t<IdType>>) {
            return resolve<T, request_interface_t<T>>(*selection.binding,
                                                      context);
        } else {
            if constexpr (cache_enabled && CheckCache) {
                if (selection.state->cache) {
                    return convert_resolved_binding<
                        request_interface_t<T>>(selection.state->cache);
                }
            }

            return resolve<T, request_interface_t<T>>(
                *selection.binding, context, *selection.state);
        }
    }

    template <typename T, bool RemoveRvalueReferences, bool MayAutoConstruct,
              typename IdType,
              typename R = as_expected_t<resolve_request_t<T, RemoveRvalueReferences>>>
    R runtime_source_missing(runtime_context& context, IdType&& id) {
        using Type = normalized_type_t<T>;
        (void)id;

        if constexpr (!std::is_same_v<void, ParentRegistry> &&
                      !std::is_same_v<void*, decltype(resolve_root_)> &&
                      !std::is_base_of_v<registry_type, resolve_root_type>) {
            if (resolve_root_) {
                if constexpr (is_none_v<std::decay_t<IdType>>) {
                    return resolve_root()
                        ->template resolve<T, RemoveRvalueReferences, true>(
                            context, none_t{});
                } else if constexpr (is_typed_key_v<IdType>) {
                    return resolve_root()
                        ->template resolve<T, RemoveRvalueReferences, true>(
                            context, std::decay_t<IdType>{});
                } else {
                    return resolve_root()->template resolve<T>(
                        std::forward<IdType>(id));
                }
            }
        }

        if constexpr (MayAutoConstruct && is_typed_key_v<IdType> &&
                      collection_traits<R>::is_collection) {
            return this->template construct_collection_runtime_request<R>(
                binding_collection_append{}, std::decay_t<IdType>{});
        } else if constexpr (MayAutoConstruct &&
                             is_auto_constructible<std::decay_t<T>>::value) {
            if constexpr (constructor<Type>::kind ==
                          constructor_kind::kConcrete) {
                return auto_construct<T>(context);
            } else if constexpr (is_none_v<std::decay_t<IdType>>) {
                return std::unexpected(
                    make_type_not_found_exception<T>(context));
            } else {
                return std::unexpected(make_type_not_found_exception<
                    T, std::decay_t<IdType>>(context));
            }
        } else if constexpr (is_none_v<std::decay_t<IdType>>) {
            return std::unexpected(
                make_type_not_found_exception<T>(context));
        } else {
            return std::unexpected(make_type_not_found_exception<T,
                                                        std::decay_t<IdType>>(
                context));
        }
    }

    template <typename T> runtime_type_bindings* runtime_collection_bindings() {
        using collection_type = collection_traits<T>;
        using resolve_type = typename collection_type::resolve_type;
        using lookup_type = normalized_type_t<resolve_type>;

        auto* state = runtime_bindings_if_present();
        return state ? state->type_bindings.template get<lookup_type>()
                     : nullptr;
    }

    template <typename Entry>
    static bool runtime_collection_entry_matches(const Entry&, none_t) {
        return true;
    }

    template <typename Entry, typename Key>
    static bool runtime_collection_entry_matches(const Entry& entry, key<Key>) {
        return entry.key_type &&
               *entry.key_type ==
                   rtti_type::template get_type_index<key<Key>>();
    }

    template <typename T, typename Fn, typename IdType>
    std::size_t append_runtime_collection(T& results, runtime_context& context,
                                          Fn&& fn, IdType id) {
        using collection_type = collection_traits<T>;
        using resolve_type = typename collection_type::resolve_type;

        auto data = runtime_collection_bindings<T>();
        if (!data) {
            return 0;
        }

        std::size_t count = 0;
        for (auto&& p : data->bindings) {
            auto& entry = p.second;
            if (!runtime_collection_entry_matches(entry, id)) {
                continue;
            }
            ++count;
            fn(results,
               resolve_collection_type<resolve_type>(*entry.binding, context));
        }

        return count;
    }

    template <typename T, typename IdType>
    std::size_t count_runtime_collection(IdType id) {
        auto data = runtime_collection_bindings<T>();
        if (!data) {
            return 0;
        }

        if constexpr (is_none_v<std::decay_t<IdType>>) {
            return data->bindings.size();
        }

        std::size_t count = 0;
        for (auto&& p : data->bindings) {
            if (runtime_collection_entry_matches(p.second, id)) {
                ++count;
            }
        }
        return count;
    }

  private:
    template <typename IdType>
    runtime_selection select_runtime_binding(runtime_type_bindings* data,
                                             IdType&& id) {
        if constexpr (is_none_v<std::decay_t<IdType>>) {
            return make_runtime_selection<
                runtime_binding_interface_type, binding_cache_state*>(
                [&](auto&& select) {
                    if (!data) {
                        return;
                    }

                    for (auto&& p : data->bindings) {
                        auto& entry = p.second;
                        select(*entry.binding, &entry);
                    }
                });
        } else if constexpr (is_typed_key_v<IdType>) {
            return make_runtime_selection<
                runtime_binding_interface_type, binding_cache_state*>(
                [&](auto&& select) {
                    if (!data) {
                        return;
                    }

                    for (auto&& p : data->bindings) {
                        auto& entry = p.second;
                        if (!entry.key_type ||
                            !(*entry.key_type ==
                              rtti_type::template get_type_index<
                                  std::decay_t<IdType>>())) {
                            continue;
                        }

                        select(*entry.binding, &entry);
                    }
                });
        } else {
            using index_key_type = std::decay_t<IdType>;
            auto indexed =
                data ? data->template get_index<index_key_type>(get_allocator())
                           .find(id)
                     : nullptr;
            return make_runtime_selection<
                runtime_binding_interface_type, binding_cache_state*>(
                indexed ? indexed->binding : nullptr, indexed);
        }
    }

    template <typename... TypeArgs, typename Parent, typename Arg,
              typename IdType>
    auto& register_type_impl(Parent* parent, Arg&& arg, IdType&& id) {
        static_assert(!has_explicit_void_interface_v<TypeArgs...>,
                      "interfaces<void> is not a valid registration target");
        using registration =
            std::conditional_t<!is_none_v<std::decay_t<Arg>>,
                               type_registration<TypeArgs..., factory<Arg>>,
                               type_registration<TypeArgs...>>;
        using binding_model = binding_model<registration>;
        using bindings_type = typename binding_model::bindings_type;
        using instance_container_type =
            registration_container_type<registration>;
        using resolution_container_type = std::conditional_t<
            std::is_void_v<bindings_type>, instance_container_type,
            binding_resolution<resolve_root_type, bindings_type>>;
        (void)arg;
        using interface_types = typename binding_model::interface_types;
        static constexpr bool storage_tag_is_complete =
            binding_model::storage_tag_is_complete;
        static_assert(storage_tag_is_complete,
                      "registered storage tag must be complete; include the "
                      "corresponding silicon/di/storage header");
        if constexpr (storage_tag_is_complete) {
            using storage_type = typename binding_model::storage_type;
            using registration_requirements =
                typename binding_model::requirements;
            using key_id_type = std::conditional_t<
                std::is_void_v<typename binding_model::key_type>, none_t,
                key<typename binding_model::key_type>>;

            registration_requirements::assert_valid();

            using runtime_binding_state_type =
                runtime_binding_state<instance_container_type, storage_type,
                                      resolution_container_type>;

            if constexpr (registration_requirements::valid &&
                          type_list_size_v<interface_types> == 1) {
                using interface_type = type_list_head_t<interface_types>;

                using runtime_binding_type = runtime_binding<
                    container_type,
                    typename annotated_traits<interface_type>::type,
                    storage_type, runtime_binding_state_type>;
                using registered_binding_type = std::conditional_t<
                    is_none_v<key_id_type>, runtime_binding_type,
                    keyed_binding_identity<key_id_type,
                                                   runtime_binding_type>>;

                if constexpr (!is_none_v<std::decay_t<Arg>>) {
                    auto&& [binding, binding_container] =
                        allocate_binding<registered_binding_type>(
                            parent, std::forward<Arg>(arg));
                    register_type_binding<interface_type, storage_type>(
                        std::move(binding), std::move(id), key_id_type{});
                    return *binding_container;
                } else {
                    auto&& [binding, binding_container] =
                        allocate_binding<registered_binding_type>(parent);
                    register_type_binding<interface_type, storage_type>(
                        std::move(binding), std::move(id), key_id_type{});
                    return *binding_container;
                }
            } else {
                if constexpr (registration_requirements::valid) {
                    std::shared_ptr<runtime_binding_state_type> data;
                    if constexpr (!is_none_v<std::decay_t<Arg>>) {
                        data = std::allocate_shared<runtime_binding_state_type>(
                            allocator_traits::rebind<
                                runtime_binding_state_type>(get_allocator()),
                            parent, std::forward<Arg>(arg));
                    } else {
                        data = std::allocate_shared<runtime_binding_state_type>(
                            allocator_traits::rebind<
                                runtime_binding_state_type>(get_allocator()),
                            parent);
                    }

                    for_each(interface_types{}, [&](auto element) {
                        using interface_type = typename decltype(element)::type;

                        using runtime_binding_type = runtime_binding<
                            container_type,
                            typename annotated_traits<interface_type>::type,
                            storage_type,
                            std::shared_ptr<runtime_binding_state_type>>;
                        using registered_binding_type = std::conditional_t<
                            is_none_v<key_id_type>, runtime_binding_type,
                            keyed_binding_identity<
                                key_id_type, runtime_binding_type>>;

                        register_type_binding<interface_type, storage_type>(
                            allocate_binding<registered_binding_type>(data)
                                .first,
                            id, key_id_type{});
                    });
                    return data->instance_container_ref();
                } else {
                    return invalid_registration_return<
                        instance_container_type>();
                }
            }
        } else {
            return invalid_registration_return<instance_container_type>();
        }
    }

    template <typename TypeInterface, typename TypeStorage, typename Binding,
              typename IdType, typename KeyIdType>
    as_expected_t<void> register_type_binding(Binding&& binding, IdType&& id, KeyIdType) {
        check_interface_requirements<
            TypeStorage, typename annotated_traits<TypeInterface>::type,
            typename TypeStorage::type>();

        auto pb =
            ensure_runtime_bindings()
                .type_bindings.template insert<TypeInterface>(
                get_allocator());
        auto& data = pb.first;
        using binding_registration_key = std::conditional_t<
            is_none_v<std::decay_t<KeyIdType>>,
            type_list<TypeInterface, typename TypeStorage::type>,
            type_list<TypeInterface, typename TypeStorage::type,
                      std::decay_t<KeyIdType>>>;

        auto inserted_binding = [&]() {
            if constexpr (is_none_v<std::decay_t<KeyIdType>>) {
                return data.bindings.template insert<binding_registration_key>(
                    std::forward<Binding>(binding), std::nullopt);
            } else {
                return data.bindings.template insert<binding_registration_key>(
                    std::forward<Binding>(binding),
                    rtti_type::template get_type_index<
                        std::decay_t<KeyIdType>>());
            }
        }();
        if (!inserted_binding.second) {
            if constexpr (is_none_v<std::decay_t<KeyIdType>>) {
                return std::unexpected(
                    make_type_already_registered_exception<
                        TypeInterface, typename TypeStorage::type>());
            } else {
                return std::unexpected(
                    make_type_index_already_registered_exception<
                        TypeInterface, typename TypeStorage::type,
                        std::decay_t<KeyIdType>>());
            }
        }
        auto binding_ptr = inserted_binding.first.binding.get();
        runtime_bindings_present_ = true;

        if constexpr (!is_none_v<std::decay_t<IdType>>) {
            if (!data.template get_index<IdType>(get_allocator())
                     .emplace(std::forward<IdType>(id),
                              index_data{binding_ptr})) {
                bool erased =
                    data.bindings.template erase<binding_registration_key>();
                assert(erased);
                (void)erased;
                return std::unexpected(
                    make_type_index_already_registered_exception<
                        TypeInterface, typename TypeStorage::type, IdType>());
            }
        }
        return {};
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
    template <typename T, bool RemoveRvalueReferences, bool MayAutoConstruct,
              bool CheckCache = true, typename IdType = none_t,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve_impl(runtime_context& context, IdType&& id = IdType()) {
        using Type = normalized_type_t<T>;
        static_assert(!std::is_const_v<Type>);

        if constexpr (cache_enabled && CheckCache) {
            if (auto* state = runtime_bindings_if_present()) {
                void* cache = state->type_cache.template get<T>();
                if (cache) {
                    return convert_resolved_binding<
                        request_interface_t<T>>(cache);
                }
            }
        }

        selected_runtime_binding<T, CheckCache, IdType> selected{*this, id};
        missing_runtime_binding<T, RemoveRvalueReferences, MayAutoConstruct,
                                IdType>
            missing{*this, id};
        auto sources = make_selected_binding_sources(selected, missing);
        return resolve_from_binding_sources<T, R>(context, sources);
    }

    template <typename T>
    decltype(auto) auto_construct(runtime_context& context) {
        using Type = normalized_type_t<T>;

        static_assert(is_complete<Type>::value,
                      "auto-construction requires a complete type");

        using type_detection = automatic;
        return context.template construct_temporary<request_interface_t<T>,
                                                    type_detection>(
            *resolve_root());
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache = true,
              typename IdType = none_t,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve(runtime_context& context, IdType&& id = IdType()) {
        return resolve_impl < T, RemoveRvalueReferences,
               std::is_same_v<request_value_t<T>, std::decay_t<T>> &&
                   (!std::is_reference_v<T> ||
                    (std::is_lvalue_reference_v<T> &&
                     std::is_const_v<std::remove_reference_t<T>> &&
                     is_auto_constructible<std::decay_t<T>>::value)),
               CheckCache > (context, std::forward<IdType>(id));
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    template <typename CachedT, typename T, typename Binding, typename Context>
    T resolve(Binding& binding, Context& context) {
        return ::silicon::di::resolve_binding_request<T, rtti_type>(
            binding, context,
            cache_enabled
                ? instance_cache_sink{this,
                                      &registry_type::store_type_cache<CachedT>}
                : instance_cache_sink{});
    }

    template <typename CachedT, typename T, typename Binding, typename Context>
    T resolve(Binding& binding, Context& context, index_data& data) {
        return ::silicon::di::resolve_binding_request<T, rtti_type>(
            binding, context,
            cache_enabled
                ? instance_cache_sink{&data, &registry_type::store_index_cache}
                : instance_cache_sink{});
    }

    template <typename CachedT, typename T, typename Binding, typename Context>
    T resolve(Binding& binding, Context& context, binding_cache_state& data) {
        return ::silicon::di::resolve_binding_request<T, rtti_type>(
            binding, context,
            cache_enabled
                ? instance_cache_sink{&data, &registry_type::store_index_cache}
                : instance_cache_sink{});
    }

    template <typename T, typename Binding, typename Context>
    T resolve_collection_type(Binding& binding, Context& context) {
        return ::silicon::di::resolve_binding_request<T, rtti_type>(binding, context);
    }

    template <class Storage, class TypeInterface, class Type>
    void check_interface_requirements() {
        interface_registration_requirements<Storage, TypeInterface,
                                                    Type>::assert_valid();
    }

    template <typename U, typename... Args>
    std::pair<runtime_binding_ptr<runtime_binding_interface<container_type>>,
              typename U::container_type*>
    allocate_binding(Args&&... args) {
        auto alloc = allocator_traits::rebind<U>(get_allocator());
        U* instance = allocator_traits::allocate(alloc, 1);
        if (!instance)
            return {nullptr, nullptr};

        allocator_traits::construct(alloc, instance,
                                    std::forward<Args>(args)...);

        return std::make_pair(
            runtime_binding_ptr<runtime_binding_interface<container_type>>(
                instance, &registry_type::template destroy_binding<U>),
            &instance->get_container());
    }

    template <typename U>
    static void
    destroy_binding(runtime_binding_interface<container_type>* ptr) {
        auto* instance = static_cast<U*>(ptr);
        auto alloc = allocator_traits::rebind<U>(
            instance->get_container().get_allocator());
        allocator_traits::destroy(alloc, instance);
        allocator_traits::deallocate(alloc, instance, 1);
    }

    resolve_root_type* resolve_root_ = nullptr;

    alignas(runtime_bindings_state) std::byte
        runtime_bindings_[sizeof(runtime_bindings_state)];
    bool runtime_bindings_constructed_ = false;

    bool runtime_bindings_present_ = false;
};

template <typename ContainerTraits, typename Allocator, typename ParentRegistry,
          typename ResolveRoot>
auto runtime_registry<ContainerTraits, Allocator, ParentRegistry,
                      ResolveRoot>::runtime_bindings_if_present()
    -> runtime_bindings_state* {
    if (!runtime_bindings_constructed_) {
        return nullptr;
    }
    return std::launder(
        reinterpret_cast<runtime_bindings_state*>(runtime_bindings_));
}

template <typename ContainerTraits, typename Allocator, typename ParentRegistry,
          typename ResolveRoot>
auto runtime_registry<ContainerTraits, Allocator, ParentRegistry,
                      ResolveRoot>::runtime_bindings_if_present() const
    -> const runtime_bindings_state* {
    if (!runtime_bindings_constructed_) {
        return nullptr;
    }
    return std::launder(
        reinterpret_cast<const runtime_bindings_state*>(runtime_bindings_));
}

template <typename ContainerTraits, typename Allocator, typename ParentRegistry,
          typename ResolveRoot>
auto runtime_registry<ContainerTraits, Allocator, ParentRegistry,
                      ResolveRoot>::ensure_runtime_bindings()
    -> runtime_bindings_state& {
    if (!runtime_bindings_constructed_) {
        new (runtime_bindings_) runtime_bindings_state(get_allocator());
        runtime_bindings_constructed_ = true;
    }
    return *std::launder(
        reinterpret_cast<runtime_bindings_state*>(runtime_bindings_));
}

template <typename ContainerTraits, typename Allocator, typename ParentRegistry,
          typename ResolveRoot>
void runtime_registry<ContainerTraits, Allocator, ParentRegistry,
                      ResolveRoot>::destroy_runtime_bindings() {
    if (runtime_bindings_constructed_) {
        std::launder(
            reinterpret_cast<runtime_bindings_state*>(runtime_bindings_))
            ->~runtime_bindings_state();
        runtime_bindings_constructed_ = false;
    }
}

} // export namespace silicon::di


// ==============================================================================================
// ==  umbrella  —  container entry points (container / runtime_container / static_container)
// ==============================================================================================

// --- runtime_container.h ---


export namespace silicon::di {

template <typename ContainerTraits = dynamic_container_traits,
          typename Allocator = typename ContainerTraits::allocator_type,
          typename ParentContainer = void>
class runtime_container
    : public runtime_registration_api<
          runtime_container<ContainerTraits, Allocator, ParentContainer>> {
    using self_type =
        runtime_container<ContainerTraits, Allocator, ParentContainer>;
    using registry_base =
        runtime_registry<ContainerTraits, Allocator, void, self_type>;

    friend class runtime_context;
    template <typename, typename> friend class binding_resolution;

  public:
    using container_traits_type = ContainerTraits;
    using allocator_type = Allocator;
    using container_type = self_type;
    using registry_type = registry_base;
    using parent_container_type =
        std::conditional_t<std::is_same_v<void, ParentContainer>, container_type,
                           ParentContainer>;
    using rtti_type = typename registry_type::rtti_type;
    using index_definition_type = typename registry_type::index_definition_type;

    template <typename ContainerTraitsT, typename AllocatorT,
              typename ParentContainerT>
    using rebind_t =
        runtime_container<ContainerTraitsT, AllocatorT, ParentContainerT>;

    template <typename Tag>
    using child_container_type =
        runtime_container<typename container_traits_type::template rebind_t<
                              type_list<typename container_traits_type::tag_type,
                                        Tag>>,
                          Allocator, container_type>;

    runtime_container() : runtime_registry_(this) {}

    explicit runtime_container(allocator_type alloc)
        : runtime_registry_(this, alloc) {}

    runtime_container(parent_container_type* parent,
                      allocator_type alloc = allocator_type())
        : runtime_registry_(this, alloc), parent_(parent) {}

    registry_type& registry() { return runtime_registry_; }

    const registry_type& registry() const { return runtime_registry_; }

    template <typename T, typename IdType = none_t,
              typename R = request_result_t<T>>
    R resolve(IdType&& id = IdType()) {
        if (parent_ &&
            runtime_registry_.template binding_status_for_id<T>(id) ==
                binding_selection_status::kNotFound) {
            if constexpr (is_none_v<std::decay_t<IdType>>) {
                return parent_->template resolve<T>();
            } else if constexpr (is_typed_key_v<IdType>) {
                return parent_->template resolve<T>(std::decay_t<IdType>{});
            } else {
                return parent_->template resolve<T>(std::forward<IdType>(id));
            }
        }
        return runtime_registry_.template resolve<T>(std::forward<IdType>(id));
    }

    template <typename T, bool RemoveRvalueReferences,
              bool CheckCache = true,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve(runtime_context& context) {
        if (parent_ &&
            runtime_registry_.template binding_status_for_id<T>(none_t{}) ==
                binding_selection_status::kNotFound) {
            return parent_->template resolve<T, RemoveRvalueReferences,
                                             CheckCache>(context);
        }
        return runtime_registry_.template resolve_request<
            T, RemoveRvalueReferences>(context);
    }

    template <typename T, bool RemoveRvalueReferences,
              bool CheckCache = true,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve(runtime_context& context, none_t) {
        return resolve<T, RemoveRvalueReferences, CheckCache>(context);
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache,
              typename Key,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve(runtime_context& context, key<Key>) {
        if (parent_ &&
            runtime_registry_.template binding_status_for_id<T>(key<Key>{}) ==
                binding_selection_status::kNotFound) {
            return parent_->template resolve<T, RemoveRvalueReferences,
                                             CheckCache>(context, key<Key>{});
        }
        return runtime_registry_.template resolve_request<
            T, RemoveRvalueReferences, Key>(context);
    }

    template <typename T,
              typename Factory = constructor<normalized_type_t<T>>,
              typename R = request_result_t<T>>
    R construct(Factory factory = Factory()) {
        return runtime_registry_.template construct<T>(std::move(factory));
    }

    template <typename T> T construct_collection() {
        return runtime_registry_.template construct_collection<T>();
    }

    template <typename T, typename Fn> T construct_collection(Fn&& fn) {
        return runtime_registry_.template construct_collection<T>(
            std::forward<Fn>(fn));
    }

    template <typename T, typename Key> T construct_collection(key<Key>) {
        return runtime_registry_.template construct_collection<T>(key<Key>{});
    }

    template <typename T, typename Fn, typename Key>
    T construct_collection(Fn&& fn, key<Key>) {
        return runtime_registry_.template construct_collection<T>(
            std::forward<Fn>(fn), key<Key>{});
    }

    template <typename T, typename Fn>
    T construct_collection(Fn&& fn, none_t) {
        return runtime_registry_.template construct_collection<T>(
            std::forward<Fn>(fn));
    }

    template <typename Signature = void, typename Callable>
    auto invoke(Callable&& callable) {
        return runtime_registry_.template invoke<Signature>(
            std::forward<Callable>(callable));
    }

  private:
    friend class runtime_registration_api<self_type>;

    registry_type& runtime_registry_ref() { return runtime_registry_; }

    self_type& runtime_registration_parent() { return *this; }

    template <typename Request, typename Key = void>
    binding_selection_status binding_status() {
        return runtime_registry_.template binding_status<Request, Key>();
    }

    template <typename T, typename Key = void, typename Fn>
    std::size_t append_collection(T& results, runtime_context& context,
                                  Fn&& fn) {
        return runtime_registry_.template append_collection<T, Key>(
            results, context, std::forward<Fn>(fn));
    }

    template <typename T, typename Key = void> std::size_t count_collection() {
        return runtime_registry_.template count_collection<T, Key>();
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve_request(runtime_context& context) {
        return runtime_registry_.template resolve_request<
            T, RemoveRvalueReferences, Key>(context);
    }

    registry_type runtime_registry_;
    parent_container_type* parent_ = nullptr;
};

} // export namespace silicon::di


// ==============================================================================
// ==  static  —  static container graph, registry & resolution
// ==============================================================================

// --- static/context.h ---



export namespace silicon::di {

template <typename StaticRegistry, bool RuntimeDependencies>
class basic_static_context;



template <typename StaticRegistry, bool RuntimeDependencies>
struct static_context_closure_selector;

template <typename StaticRegistry>
struct static_context_closure_selector<StaticRegistry, false> {
    using execution_traits =
        basic_static_execution_traits<StaticRegistry, false>;
    using type = static_context_closure<
        execution_traits::max_destructible_slots,
        execution_traits::max_temporary_slots,
        execution_traits::max_temporary_size == 0
            ? 1
            : execution_traits::max_temporary_size,
        execution_traits::max_temporary_align == 0
            ? alignof(std::max_align_t)
            : execution_traits::max_temporary_align>;
};

template <typename StaticRegistry>
struct static_context_closure_selector<StaticRegistry, true> {
    using execution_traits =
        basic_static_execution_traits<StaticRegistry, true>;
    using type = fixed_context_closure<
        execution_traits::max_destructible_slots,
        execution_traits::max_temporary_slots,
        execution_traits::max_temporary_size == 0
            ? 1
            : execution_traits::max_temporary_size,
        execution_traits::max_temporary_align == 0
            ? alignof(std::max_align_t)
            : execution_traits::max_temporary_align>;
};

template <typename StaticRegistry>
using binding_context = basic_static_context<StaticRegistry, true>;



template <typename StaticRegistry, bool RuntimeDependencies = false>
class basic_static_context : public context_path_state {
    using execution_traits =
        basic_static_execution_traits<StaticRegistry,
                                              RuntimeDependencies>;
    static constexpr std::size_t closure_capacity_ =
        execution_traits::max_preserved_closure_depth + 1;
    static constexpr std::size_t destructible_capacity_ =
        execution_traits::max_destructible_slots;
    static constexpr std::size_t temporary_slot_capacity_ =
        execution_traits::max_temporary_slots;
    static constexpr std::size_t temporary_slot_size_ =
        execution_traits::max_temporary_size == 0
            ? 1
            : execution_traits::max_temporary_size;
    static constexpr std::size_t temporary_slot_align_ =
        execution_traits::max_temporary_align == 0
            ? alignof(std::max_align_t)
            : execution_traits::max_temporary_align;
    using closure_type = typename static_context_closure_selector<
        StaticRegistry, RuntimeDependencies>::type;

  public:
    basic_static_context() { closures_[0] = &closure_; }

    ~basic_static_context() {
        while (closure_count_ != 0) {
            closures_[--closure_count_]->reset();
        }
    }

    template <typename T, typename Container> T resolve(Container& container) {
        if constexpr (is_keyed_v<T>) {
            using request_type = keyed_type_t<T>;
            using key_type = keyed_key_t<T>;
            return T(container.template resolve<request_type, false, true>(
                *this, key<key_type>{}));
        } else {
            return container.template resolve<T, false>(*this);
        }
    }

    template <typename T, typename DetectionTag, typename Container>
    T construct_temporary(Container& container) {
        using temporary_type = normalized_type_t<T>;

        auto* instance = allocate_temporary_storage<temporary_type>();
        default_constructor_detection<temporary_type, DetectionTag>()
            .template construct<temporary_type>(instance, *this, container);
        if constexpr (!std::is_trivially_destructible_v<temporary_type>) {
            register_destructor(instance);
        }

        if constexpr (std::is_lvalue_reference_v<T>) {
            return *instance;
        } else {
            return std::move(*instance);
        }
    }

    template <typename T, typename... Args> T& construct(Args&&... args) {
        auto* instance = allocate_temporary_storage<T>();
        new (instance) T(std::forward<Args>(args)...);
        if constexpr (!std::is_trivially_destructible_v<T>) {
            register_destructor(instance);
        }
        return *instance;
    }

    template <typename T> T* allocate() {
        return allocate_temporary_storage<T>();
    }

    void push(closure_type* closure) {
        assert(!contains(closure));
        assert(closure_count_ < closures_.size());
        closures_[closure_count_++] = closure;
    }

    void pop() {
        assert(closure_count_ != 0);
        --closure_count_;
    }

    bool contains(const closure_type* candidate) const {
        for (std::size_t index = 0; index < closure_count_; ++index) {
            if (closures_[index] == candidate) {
                return true;
            }
        }
        return false;
    }

  private:
    template <typename T> T* allocate_temporary_storage() {
        static_assert(sizeof(T) <= temporary_slot_size_,
                      "static_context temporary size must fit the compile-time "
                      "temporary slot bound");
        static_assert(alignof(T) <= temporary_slot_align_,
                      "static_context temporary alignment must fit the "
                      "compile-time temporary slot bound");
        static_assert(temporary_slot_capacity_ != 0,
                      "static_context requires at least one compile-time "
                      "temporary slot for this resolution path");

        auto* fixed = active_closure().template try_allocate_temporary<T>();
        assert(fixed != nullptr);
        return fixed;
    }

    closure_type& active_closure() {
        assert(closure_count_ != 0);
        return *closures_[closure_count_ - 1];
    }

    template <typename T> void register_destructor(T* instance) {
        static_assert(!std::is_trivially_destructible_v<T>);
        active_closure().add_destructor(instance, &destructor<T>);
    }

    template <typename T> static void destructor(void* ptr) {
        reinterpret_cast<T*>(ptr)->~T();
    }

    std::array<closure_type*, closure_capacity_> closures_{};
    std::size_t closure_count_ = 1;
    closure_type closure_;
};

template <typename StaticRegistry>
using static_context = basic_static_context<StaticRegistry, false>;

} // export namespace silicon::di

// --- static/container_traits.h ---



export namespace silicon::di {

template <typename Tag = void> struct static_container_traits {
    template <typename TagT> using rebind_t = static_container_traits<TagT>;

    using tag_type = Tag;
    using rtti_type = rtti<static_provider>;
    template <typename Value, typename Allocator>
    using type_map_type = static_type_map<Value, Tag, Allocator>;
    template <typename Value, typename Allocator>
    using type_cache_type = static_type_cache<void*, Tag, Allocator>;
    using allocator_type = static_allocator<char, Tag>;
    using index_definition_type = std::tuple<>;
    static constexpr bool cache_enabled = true;
};

template <typename StaticSource, typename ParentContainer = void>
class static_container;



template <typename StaticRegistry, typename ParentContainer = void>
class container_with_static_bindings;

template <typename T> struct is_static_registry : std::false_type {};

template <typename... Registrations>
struct is_static_registry<static_registry<Registrations...>> : std::true_type {};

template <typename T>
inline constexpr bool is_static_registry_v = is_static_registry<T>::value;

template <typename T> struct is_bindings_wrapper : std::false_type {};

template <typename... Args>
struct is_bindings_wrapper<::silicon::di::bindings<Args...>> : std::true_type {};

template <typename T>
inline constexpr bool is_bindings_wrapper_v = is_bindings_wrapper<T>::value;

template <typename T> struct bindings_wrapper_registry;

template <typename... Args>
struct bindings_wrapper_registry<::silicon::di::bindings<Args...>> {
    using type = typename ::silicon::di::bindings<Args...>::type;
};

template <typename T>
using bindings_wrapper_registry_t = typename bindings_wrapper_registry<T>::type;


} // export namespace silicon::di

// --- static/resolution.h ---



export namespace silicon::di {


template <typename StaticRegistry, bool DependenciesResolved>
struct static_container_graph_type;

template <typename StaticRegistry>
struct static_container_graph_type<StaticRegistry, true> {
    using type = static_graph<StaticRegistry>;
};

template <typename StaticRegistry>
struct static_container_graph_type<StaticRegistry, false> {
    struct type {
        static constexpr bool resolvable = true;
        static constexpr bool acyclic = true;
    };
};

struct factory_probe_context {
    template <typename T, typename Container> T resolve(Container&) = delete;
};

struct factory_probe_container {};

template <typename Factory>
inline constexpr bool factory_has_no_dependencies_v =
    std::is_same_v<typename factory_traits<Factory>::dependencies, type_list<>>;

template <typename Factory, typename Type>
Type construct_factory_value_without_dependencies() {
    factory_probe_context context;
    factory_probe_container container;
    return Factory::template construct<Type>(context, container);
}

template <typename Selection, typename Request,
          bool Enabled = Selection::status == binding_selection_status::kFound>
struct binding_factory {
    static constexpr bool enabled = false;
};

template <typename Selection, typename Request>
struct binding_factory<Selection, Request, true> {
    using binding_type = typename Selection::binding_type;
    using factory_type =
        typename binding_type::binding_model_type::factory_type;

    static constexpr bool enabled = factory_has_no_dependencies_v<factory_type>;

    static normalized_type_t<Request> construct() {
        return construct_factory_value_without_dependencies<
            factory_type, normalized_type_t<Request>>();
    }
};

template <typename Request, typename Selection, typename ResolveNormalized>
request_result_t<Request>
construct_static_binding_value(ResolveNormalized&& resolve_normalized) {
    if constexpr (binding_factory<Selection, Request>::enabled) {
        return type_traits<std::decay_t<Request>>::make(
            binding_factory<Selection, Request>::construct());
    } else {
        auto&& value = std::forward<ResolveNormalized>(resolve_normalized)();
        return type_traits<std::decay_t<Request>>::make(
            std::forward<decltype(value)>(value));
    }
}


} // export namespace silicon::di


// ==============================================================================
// ==  storage  —  storage policies (shared / unique / external / cyclical)
// ==============================================================================

// --- storage/resettable.h ---


export namespace silicon::di {
// 鸭子类型即满足，无需 proxy 门面（无类型擦除消费者）。
} // export namespace silicon::di

// --- storage/external.h ---



export namespace silicon::di {
struct external {};

template <typename Type> struct storage_materialization_traits<external, Type> {
    template <typename Leaf, typename Context, typename Storage>
    static auto make_guard(Context&, const Storage&) {
        return no_materialization_scope();
    }

    template <typename Storage>
    static bool preserves_closure(const Storage&) {
        return false;
    }

    template <typename Context, typename Storage, typename Container>
    static auto materialize_source(Context& context, Storage& storage,
                                   Container& container) {
        return make_resolved_source(storage.resolve(context, container));
    }
};

template <typename Type, typename U>
struct storage_traits<
    external, Type, U,
    std::enable_if_t<!type_traits<Type>::enabled && !std::is_reference_v<Type> &&
                     !std::is_array_v<Type>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using value_types = type_list<U>;
    using lvalue_reference_types = type_list<U&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<U*>;
    using conversion_types = type_list<>;
};

template <typename Type, typename U>
struct storage_traits<external, Type*, U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using value_types = type_list<U>;
    using lvalue_reference_types = type_list<U&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<U*>;
    using conversion_types = type_list<>;
};

template <typename T, typename U>
struct storage_traits<external, T[], U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using value_types = type_list<>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<typename wrapper_rebind_leaf<T, U>::type*>;
    using conversion_types = type_list<>;
};

template <typename T, size_t N, typename U>
struct storage_traits<external, T[N], U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using rebound_row_type = typename wrapper_rebind_leaf<T, U>::type;
    using rebound_exact_type =
        typename wrapper_rebind_leaf<T[N], U>::type;

    using value_types = type_list<>;
    using lvalue_reference_types = type_list<exact_lookup<rebound_exact_type>&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types =
        type_list<rebound_row_type*, exact_lookup<rebound_exact_type>*>;
    using conversion_types = type_list<>;
};

template <typename Array, typename Deleter, typename U>
struct storage_traits<
    external, std::unique_ptr<Array, Deleter>, U,
    std::enable_if_t<std::is_array_v<Array> && (std::extent_v<Array, 0> == 0)>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using handle_type =
        wrapper_rebind_leaf_t<std::unique_ptr<Array, Deleter>, U>;
    using pointer_types =
        typename smart_array_pointer_types<
            handle_type, std::remove_extent_t<Array>, U>::type;

    using value_types = type_list<>;
    using lvalue_reference_types = type_list<handle_type&>;
    using rvalue_reference_types = type_list<>;
    using conversion_types = type_list<>;
};

template <typename T, typename Deleter, typename U>
struct storage_traits<external, std::unique_ptr<T, Deleter>, U,
                      std::enable_if_t<!std::is_array_v<T>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using handle_type =
        wrapper_rebind_leaf_t<std::unique_ptr<T, Deleter>, U>;
    using types = wrapper_storage_types<handle_type>;

    using value_types = type_list<>;
    using lvalue_reference_types = typename types::lvalue_reference_types;
    using rvalue_reference_types = type_list<>;
    using pointer_types = typename types::pointer_types;
    using conversion_types = type_list<>;
};

template <typename Array, typename U>
struct storage_traits<
    external, std::shared_ptr<Array>, U,
    std::enable_if_t<std::is_array_v<Array> && (std::extent_v<Array, 0> == 0)>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using handle_type = wrapper_rebind_leaf_t<std::shared_ptr<Array>, U>;
    using pointer_types =
        typename smart_array_pointer_types<
            handle_type, std::remove_extent_t<Array>, U>::type;

    using value_types = type_list<handle_type>;
    using lvalue_reference_types = type_list<handle_type&>;
    using rvalue_reference_types = type_list<>;
    using conversion_types = type_list<handle_type>;
};

template <typename T, typename U>
struct storage_traits<external, std::shared_ptr<T>, U,
                      std::enable_if_t<!std::is_array_v<T>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using handle_type = wrapper_rebind_leaf_t<std::shared_ptr<T>, U>;
    using types = wrapper_storage_types<handle_type>;

    using value_types = typename types::copyable_value_types;
    using lvalue_reference_types = typename types::lvalue_reference_types;
    using rvalue_reference_types = type_list<>;
    using pointer_types = typename types::pointer_types;
    using conversion_types = typename types::copyable_value_types;
};

template <typename T, typename U>
struct storage_traits<external, std::optional<T>, U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using value_types = type_list<>;
    using lvalue_reference_types =
        type_list<U&, exact_lookup<std::optional<T>>&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<U*, exact_lookup<std::optional<T>>*>;
    using conversion_types = type_list<>;
};


template <typename Type, typename U> struct conversions<external, Type, U>
    : type_storage_traits<external, Type, U> {};

template <typename Type, typename U> struct conversions<external, Type&, U>
    : public type_storage_traits<external, Type&, U> {};

template <typename Type, typename U> struct conversions<external, Type*, U>
    : public type_storage_traits<external, Type*, U> {};

template <typename Type, typename StoredType, typename = void>
class external_storage_instance_impl {
  public:
    template <typename T>
    external_storage_instance_impl(T&& instance)
        : instance_(std::forward<T>(instance)) {}

    Type& get() { return instance_; }

  private:
    Type instance_;
};

template <typename Type, typename StoredType>
class external_storage_instance_impl<
    Type, StoredType, std::enable_if_t<type_traits<Type>::enabled>> {
  public:
    template <typename T>
    external_storage_instance_impl(T&& instance)
        : instance_(type_conversion_traits<StoredType, Type>::convert(
              std::forward<T>(instance))) {}

    StoredType& get() { return instance_; }

  private:
    StoredType instance_;
};

template <typename Type, typename StoredType>
class storage_instance<external, Type, StoredType, void>
    : public external_storage_instance_impl<Type, StoredType> {
  public:
    template <typename T>
    storage_instance(T&& instance)
        : external_storage_instance_impl<Type, StoredType>(
              std::forward<T>(instance)) {}
};

template <typename Type, size_t N, typename StoredType>
class storage_instance<external, Type[N], StoredType, void> {
  public:
    storage_instance(Type (&instance)[N]) : instance_(instance) {}

    Type* get() { return instance_; }

  private:
    Type (&instance_)[N];
};

template <typename Type, typename StoredType>
class storage_instance<external, Type&, StoredType, void> {
  public:
    storage_instance(Type& instance) : instance_(instance) {}

    Type& get() { return instance_; }

  private:
    Type& instance_;
};

template <typename Type, typename StoredType>
class storage_instance<external, Type*, StoredType, void> {
  public:
    storage_instance(Type* instance) : instance_(instance) {}

    Type* get() { return instance_; }

  private:
    Type* instance_;
};

template <typename Type, typename StoredType, typename Factory,
          typename Conversions>
class storage<external, Type, StoredType, Factory, Conversions>
    {
    storage_instance<external, Type, StoredType, void> instance_;

  public:
    using conversions = Conversions;
    using type = Type;
    using stored_type = StoredType;
    using tag_type = external;

    template <typename T>
    storage(T&& instance) : instance_(std::forward<T>(instance)) {}

    template <typename Context, typename Container>
    decltype(auto) resolve(Context&, Container&) {
        return instance_.get();
    }
    constexpr bool is_resolved() const { return true; }

    void reset() {}
};

} // export namespace silicon::di

// --- storage/shared.h ---




export namespace silicon::di {
struct shared {};

template <typename Type> struct storage_materialization_traits<shared, Type> {
    template <typename Leaf, typename Context, typename Storage>
    static auto make_guard(Context& context, const Storage& storage) {
        return recursion_guard_wrapper<Leaf>(
            context, !storage.is_resolved());
    }

    template <typename Storage>
    static bool preserves_closure(const Storage& storage) {
        // Only unresolved shared storage needs the factory closure to stay on
        // the active context stack while address-based conversions are
        // materialized. Once the instance is resolved, there are no temporary
        // construction artifacts left to preserve.
        return !storage.is_resolved();
    }

    template <typename Context, typename Storage, typename Container>
    static auto materialize_source(Context& context, Storage& storage,
                                   Container& container) {
        return make_resolved_source(storage.resolve(context, container));
    }
};

template <typename Type, typename U>
struct storage_traits<
    shared, Type, U,
    std::enable_if_t<!type_traits<Type>::enabled && !std::is_reference_v<Type> &&
                     !std::is_array_v<Type>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using value_types = type_list<U>;
    using lvalue_reference_types = type_list<U&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<U*>;
    using conversion_types = type_list<>;
};

template <typename Type, typename U>
struct storage_traits<shared, Type*, U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using value_types = type_list<>;
    using lvalue_reference_types = type_list<U&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<U*>;
    using conversion_types = type_list<>;
};

template <typename T, typename U>
struct storage_traits<shared, T[], U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using value_types = type_list<>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<typename wrapper_rebind_leaf<T, U>::type*>;
    using conversion_types = type_list<>;
};

template <typename T, size_t N, typename U>
struct storage_traits<shared, T[N], U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using rebound_row_type = typename wrapper_rebind_leaf<T, U>::type;
    using rebound_exact_type =
        typename wrapper_rebind_leaf<T[N], U>::type;

    using value_types = type_list<>;
    using lvalue_reference_types = type_list<exact_lookup<rebound_exact_type>&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types =
        type_list<rebound_row_type*, exact_lookup<rebound_exact_type>*>;
    using conversion_types = type_list<>;
};

template <typename Array, typename Deleter, typename U>
struct storage_traits<
    shared, std::unique_ptr<Array, Deleter>, U,
    std::enable_if_t<std::is_array_v<Array> && (std::extent_v<Array, 0> == 0)>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using handle_type =
        wrapper_rebind_leaf_t<std::unique_ptr<Array, Deleter>, U>;
    using pointer_types =
        typename smart_array_pointer_types<
            handle_type, std::remove_extent_t<Array>, U>::type;

    using value_types = type_list<>;
    using lvalue_reference_types = type_list<handle_type&>;
    using rvalue_reference_types = type_list<>;
    using conversion_types = type_list<>;
};

template <typename T, typename Deleter, typename U>
struct storage_traits<shared, std::unique_ptr<T, Deleter>, U,
                      std::enable_if_t<!std::is_array_v<T>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using handle_type =
        wrapper_rebind_leaf_t<std::unique_ptr<T, Deleter>, U>;
    using types = wrapper_storage_types<handle_type>;

    using value_types = type_list<U>;
    using lvalue_reference_types = typename types::lvalue_reference_types;
    using rvalue_reference_types = type_list<>;
    using pointer_types = typename types::pointer_types;
    using conversion_types = type_list<>;
};

template <typename Array, typename U>
struct storage_traits<
    shared, std::shared_ptr<Array>, U,
    std::enable_if_t<std::is_array_v<Array> && (std::extent_v<Array, 0> == 0)>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using handle_type = wrapper_rebind_leaf_t<std::shared_ptr<Array>, U>;
    using pointer_types =
        typename smart_array_pointer_types<
            handle_type, std::remove_extent_t<Array>, U>::type;

    using value_types = type_list<handle_type>;
    using lvalue_reference_types = type_list<handle_type&>;
    using rvalue_reference_types = type_list<>;
    using conversion_types = type_list<handle_type>;
};

template <typename T, typename U>
struct storage_traits<shared, std::shared_ptr<T>, U,
                      std::enable_if_t<!std::is_array_v<T>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using handle_type = wrapper_rebind_leaf_t<std::shared_ptr<T>, U>;
    using types = wrapper_storage_types<handle_type>;

    using value_types =
        type_list_cat_t<type_list<U>, typename types::copyable_value_types>;
    using lvalue_reference_types = typename types::lvalue_reference_types;
    using rvalue_reference_types = type_list<>;
    using pointer_types = typename types::pointer_types;
    using conversion_types = typename types::copyable_value_types;
};

template <typename T, typename U>
struct storage_traits<shared, std::optional<T>, U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = true;

    using value_types = type_list<>;
    using lvalue_reference_types =
        type_list<U&, exact_lookup<std::optional<T>>&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<U*, exact_lookup<std::optional<T>>*>;
    using conversion_types = type_list<>;
};


template <typename Type, typename U> struct conversions<shared, Type, U>
    : type_storage_traits<shared, Type, U> {};

template <typename Type, typename Factory>
struct storage_instance_base : Factory {
    template <typename... Args>
    storage_instance_base(Args&&... args)
        : Factory(std::forward<Args>(args)...) {}

    Type* get() const {
        return std::launder(reinterpret_cast<Type*>(&instance_));
    }

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
    template <typename Context, typename Container>
    void construct(Context& context, Container& container) {
        assert(!initialized_);
        Factory::template construct<Type*>(&instance_, context, container);
        initialized_ = true;
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    bool empty() const { return !initialized_; }

  protected:
    mutable silicon::di::aligned_storage_t<sizeof(Type), alignof(Type)> instance_;
    bool initialized_ = false;
};

template <typename Type, typename Factory,
          bool IsTriviallyDestructible = std::is_trivially_destructible_v<Type>>
struct storage_instance_dtor;

template <typename Type, typename Factory>
struct storage_instance_dtor<Type, Factory, true>
    : storage_instance_base<Type, Factory> {
    template <typename... Args>
    storage_instance_dtor(Args&&... args)
        : storage_instance_base<Type, Factory>(std::forward<Args>(args)...) {}

    void reset() { this->initialized_ = false; }
};

template <typename Type, typename Factory>
struct storage_instance_dtor<Type, Factory, false>
    : storage_instance_base<Type, Factory> {
    template <typename... Args>
    storage_instance_dtor(Args&&... args)
        : storage_instance_base<Type, Factory>(std::forward<Args>(args)...) {}

    ~storage_instance_dtor() { reset(); }

    void reset() {
        if (this->initialized_) {
            this->initialized_ = false;
            this->get()->~Type();
        }
    }
};

template <typename Type, typename StoredType, typename Factory,
          typename = void>
class shared_storage_instance_impl : public storage_instance_dtor<Type, Factory> {
  public:
    template <typename... Args>
    shared_storage_instance_impl(Args&&... args)
        : storage_instance_dtor<Type, Factory>(std::forward<Args>(args)...) {}

    static_assert(
        std::is_trivially_destructible_v<Type> ==
        std::is_trivially_destructible_v<storage_instance_dtor<Type, Factory>>);
};

template <typename Type, typename StoredType, typename Factory>
class shared_storage_instance_impl<
    Type, StoredType, Factory, std::enable_if_t<type_traits<Type>::enabled>>
    : Factory {
  public:
    template <typename... Args>
    shared_storage_instance_impl(Args&&... args)
        : Factory(std::forward<Args>(args)...) {}

    template <typename Context, typename Container>
    void construct(Context& context, Container& container) {
        assert(empty());
        new (&instance_) StoredType(
            type_conversion_traits<StoredType, Type>::convert(
                Factory::template construct<Type>(context, container)));
        initialized_ = true;
    }

    ~shared_storage_instance_impl() { reset(); }

    StoredType& get() const { return *get_ptr(); }

    void reset() {
        if (initialized_) {
            get_ptr()->~StoredType();
            initialized_ = false;
        }
    }

    bool empty() const { return !initialized_; }

  private:
    StoredType* get_ptr() const {
        return std::launder(reinterpret_cast<StoredType*>(&instance_));
    }

    mutable aligned_storage_t<sizeof(StoredType), alignof(StoredType)> instance_;
    bool initialized_ = false;
};

template <typename Type, typename StoredType, typename Factory>
class storage_instance<shared, Type, StoredType, Factory>
    : public shared_storage_instance_impl<Type, StoredType, Factory> {
  public:
    template <typename... Args>
    storage_instance(Args&&... args)
        : shared_storage_instance_impl<Type, StoredType, Factory>(
              std::forward<Args>(args)...) {}
};

template <typename Type, size_t N, typename StoredType, typename Factory>
class storage_instance<shared, Type[N], StoredType, Factory>
    : Factory {
  public:
    template <typename... Args>
    storage_instance(Args&&... args)
        : Factory(std::forward<Args>(args)...) {}

    ~storage_instance() { reset(); }

    template <typename Context, typename Container>
    void construct(Context& context, Container& container) {
        assert(empty());
        Factory::template construct<Type[N]>(&instance_, context, container);
        initialized_ = true;
    }

    Type* get() const {
        return std::addressof((*get_array())[0]);
    }

    void reset() {
        if (!initialized_) {
            return;
        }

        destroy_object_value(*get_array());
        initialized_ = false;
    }

    bool empty() const { return !initialized_; }

  private:
    Type (*get_array() const)[N] {
        return std::launder(reinterpret_cast<Type(*)[N]>(&instance_));
    }

    mutable aligned_storage_t<sizeof(Type[N]), alignof(Type[N])> instance_;
    bool initialized_ = false;
};

template <typename Type, typename StoredType, typename Factory>
class storage_instance<shared, Type*, StoredType*, Factory>
    : Factory {
  public:
    template <typename... Args>
    storage_instance(Args&&... args) : Factory(std::forward<Args>(args)...) {}

    ~storage_instance() { reset(); }

    template <typename Context, typename Container>
    void construct(Context& context, Container& container) {
        assert(empty());
        instance_ = Factory::template construct<Type*>(context, container);
    }

    StoredType* get() const {
        return type_conversion_traits<StoredType*, Type*>::convert(instance_);
    }
    void reset() {
        delete instance_;
        instance_ = nullptr;
    }
    bool empty() const { return instance_ == nullptr; }

  private:
    Type* instance_ = nullptr;
};

template <typename Type, typename StoredType, typename Factory,
          typename Conversions>
class storage<shared, Type, StoredType, Factory, Conversions>
    {
    // TODO
    // static_assert(std::is_trivially_destructible_v< Type > ==
    // std::is_trivially_destructible_v< storage_instance< Type, shared > >);
    storage_instance<shared, Type, StoredType, Factory> instance_;

  public:
    template <typename... Args>
    storage(Args&&... args) : instance_(std::forward<Args>(args)...) {}

    using conversions = Conversions;
    using type = Type;
    using stored_type = StoredType;
    using tag_type = shared;

    template <typename Context, typename Container>
    decltype(auto) resolve(Context& context, Container& container) {
        if (instance_.empty())
            instance_.construct(context, container);
        return instance_.get();
    }

    bool is_resolved() const { return !instance_.empty(); }
    void reset() { instance_.reset(); }
};

} // export namespace silicon::di

// --- storage/shared_cyclical.h ---




export namespace silicon::di {
struct shared_cyclical {};

template <typename Type>
struct storage_materialization_traits<shared_cyclical, Type> {
    template <typename Leaf, typename Context, typename Storage>
    static auto make_guard(Context&, const Storage&) {
        return no_materialization_scope();
    }

    template <typename Storage>
    static bool preserves_closure(const Storage&) {
        return false;
    }

    template <typename Context, typename Storage, typename Container>
    static auto materialize_source(Context& context, Storage& storage,
                                   Container& container) {
        return make_resolved_source(storage.resolve(context, container));
    }
};

template <typename Type, typename U>
struct storage_traits<
    shared_cyclical, Type, U,
    std::enable_if_t<!type_traits<Type>::enabled && !std::is_pointer_v<Type> &&
                     !std::is_reference_v<Type>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using value_types = type_list<>;
    using lvalue_reference_types = type_list<U&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<U*>;
    using conversion_types = type_list<>;
};

template <typename Type, typename U>
struct storage_traits<shared_cyclical, Type*, U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using value_types = type_list<>;
    using lvalue_reference_types = type_list<U&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<U*>;
    using conversion_types = type_list<>;
};

template <typename Type, typename U>
struct storage_traits<shared_cyclical, std::shared_ptr<Type>, U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using value_types = type_list<std::shared_ptr<U>>;
    using lvalue_reference_types = type_list<U&, std::shared_ptr<U>&>;
    using rvalue_reference_types = type_list<>;
    using pointer_types = type_list<U*, std::shared_ptr<U>*>;
    using conversion_types = type_list<std::shared_ptr<U>>;
};

template <typename Base, typename Derived> struct is_virtual_base_of {
#if defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winaccessible-base"
#endif
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4250)
#endif
    struct test : Derived, virtual Base {};
#if defined(__GNUG__)
#pragma GCC diagnostic pop
#endif
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    // If this equals, it means Base is already a virtual base of Derived
    static constexpr bool value = sizeof(test) == sizeof(Derived);
};

template <typename Base, typename Derived>
inline constexpr bool is_virtual_base_of_v =
    is_virtual_base_of<Base, Derived>::value;


template <typename Type, typename U> struct conversions<shared_cyclical, Type, U>
    : type_storage_traits<shared_cyclical, Type, U> {};

// Disallow virtual bases as interfaces as in cyclical storage, we can't
// properly calculate the cast when the object is not constructed
template <typename Type, typename StoredType, typename Factory,
          typename Conversions, typename Derived, typename Base>
struct storage_interface_requirements<
    storage<shared_cyclical, Type, StoredType, Factory, Conversions>, Derived,
    Base> : std::bool_constant<!is_virtual_base_of_v<Base, Derived>> {};

template <typename Type, typename Factory,
          bool IsTriviallyDestructible = std::is_trivially_destructible_v<Type>>
class cyclical_storage_instance_impl;

template <typename Type, typename Factory>
class cyclical_storage_instance_impl<Type, Factory, false> : Factory {
  public:
    template <typename... Args>
    cyclical_storage_instance_impl(Args&&... args)
        : Factory(std::forward<Args>(args)...) {}

    ~cyclical_storage_instance_impl() { reset(); }

    template <typename Context> Type* resolve(Context&) {
        assert(!resolved_);
        resolved_ = true;
        return get();
    }

    Type* get() { return std::launder(reinterpret_cast<Type*>(&instance_)); }

    template <typename Context, typename Container>
    void construct(Context& context, Container& container) {
        assert(resolved_);
        Factory::template construct<Type*>(&instance_, context, container);
        constructed_ = true;
    }

    bool empty() const { return !resolved_; }

    void reset() {
        resolved_ = false;
        if (constructed_) {
            constructed_ = false;
            std::launder(reinterpret_cast<Type*>(&instance_))->~Type();
        }
    }

  private:
    silicon::di::aligned_storage_t<sizeof(Type), alignof(Type)> instance_;
    bool resolved_ = false;
    bool constructed_ = false;
};

template <typename Type, typename Factory>
class cyclical_storage_instance_impl<Type, Factory, true> : Factory {
  public:
    template <typename... Args>
    cyclical_storage_instance_impl(Args&&... args)
        : Factory(std::forward<Args>(args)...) {}

    template <typename Context> Type* resolve(Context&) {
        assert(!resolved_);
        resolved_ = true;
        return get();
    }

    Type* get() { return std::launder(reinterpret_cast<Type*>(&instance_)); }

    template <typename Context, typename Container>
    void construct(Context& context, Container& container) {
        assert(resolved_);
        Factory::template construct<Type*>(&instance_, context, container);
    }

    bool empty() const { return !resolved_; }

    void reset() { resolved_ = false; }

  private:
    silicon::di::aligned_storage_t<sizeof(Type), alignof(Type)> instance_;
    bool resolved_ = false;
};

template <typename Type, typename StoredType, typename Factory>
class storage_instance<shared_cyclical, Type, StoredType, Factory>
    : public cyclical_storage_instance_impl<Type, Factory> {
  public:
    template <typename... Args>
    storage_instance(Args&&... args)
        : cyclical_storage_instance_impl<Type, Factory>(
              std::forward<Args>(args)...) {}
};

// This path is intentionally std::shared_ptr-specific. We need to publish a
// copyable owning handle before the pointee is constructed so cyclical
// dependencies can observe a stable address during construction.
template <typename Type, typename StoredType, typename Factory>
class storage_instance<shared_cyclical, std::shared_ptr<Type>,
                       std::shared_ptr<StoredType>, Factory> : Factory {
    using storage_type = silicon::di::aligned_storage_t<sizeof(Type), alignof(Type)>;
    
    class deleter {
      public:
        void set_constructed() { constructed_ = true; }

        void operator()(Type* ptr) {
            if constexpr (!std::is_trivially_destructible_v<Type>) {
                if (constructed_)
                    ptr->~Type();
            }

            delete reinterpret_cast<storage_type*>(ptr);
        }

      private:
        bool constructed_ = false;
    };

  public:
    template <typename... Args>
    storage_instance(Args&&... args) : Factory(std::forward<Args>(args)...) {}

    template <typename Context> std::shared_ptr<StoredType>& resolve(Context&) {
        assert(!instance_);
        instance_.reset(reinterpret_cast<Type*>(new storage_type), deleter());
        return instance_;
    }

    std::shared_ptr<StoredType>& get() { return instance_; }

    template <typename Context, typename Container>
    void construct(Context& context, Container& container) {
        assert(instance_);
        Factory::template construct<Type*>(instance_.get(), context, container);
        std::get_deleter<deleter>(instance_)->set_constructed();
    }

    bool empty() const { return !instance_; }

    void reset() { instance_.reset(); }

  private:
    std::shared_ptr<StoredType> instance_;
};

template <typename Type, typename StoredType, typename Factory,
          typename Conversions>
class storage<shared_cyclical, Type, StoredType, Factory, Conversions>
    {
    struct conversion_entry {
        explicit conversion_entry(type_descriptor key) : descriptor(key) {}
        virtual ~conversion_entry() = default;

        type_descriptor descriptor;
    };

    template <typename T> struct typed_conversion_entry : conversion_entry {
        template <typename Source>
        explicit typed_conversion_entry(Source&& source)
            : conversion_entry(describe_type<T>()),
              value(std::forward<Source>(source)) {}

        T value;
    };

    storage_instance<shared_cyclical, Type, StoredType, Factory> instance_;
    std::vector<std::unique_ptr<conversion_entry>> conversions_;

    struct rollback {
        explicit rollback(storage* storage) : storage_(storage) {}
        ~rollback() {
            if (std::uncaught_exceptions())
                storage_->reset();
        }

      private:
        storage* storage_;
    };

  public:
    template <typename... Args>
    storage(Args&&... args) : instance_(std::forward<Args>(args)...) {}

    using conversions = Conversions;
    using type = Type;
    using stored_type = StoredType;
    using tag_type = shared_cyclical;

    template <typename Context, typename Container>
    decltype(auto) resolve(Context& context, Container& container) {
        if (instance_.empty()) {
            // shared_cyclical publishes its owning handle before the pointee
            // is fully constructed so the graph can close over a stable
            // address. Rebound shared_ptr interface handles therefore belong
            // to the storage too and must roll back with the instance if the
            // first resolve throws.
            context.template construct<rollback>(this);

            instance_.resolve(context);
            instance_.construct(context, container);
            return instance_.get();
        }
        return instance_.get();
    }

    template <typename T, typename Context, typename Source>
    T& resolve_conversion(Context&, Source&& source) {
        for (auto& entry : conversions_) {
            if (entry->descriptor == describe_type<T>()) {
                return static_cast<typed_conversion_entry<T>&>(*entry).value;
            }
        }

        conversions_.push_back(
            std::make_unique<typed_conversion_entry<T>>(
                std::forward<Source>(source)));
        return static_cast<typed_conversion_entry<T>&>(*conversions_.back())
            .value;
    }

    bool is_resolved() const { return !instance_.empty(); }

    void reset() {
        // Graph objects can keep references to rebound shared_ptr interface
        // handles stored in `conversions_`. Destroy the object graph first so
        // those references stay valid for any destructor work.
        instance_.reset();
        conversions_.clear();
    }
};

} // export namespace silicon::di

// --- storage/unique.h ---



export namespace silicon::di {
struct unique {};

template <typename Type> struct storage_materialization_traits<unique, Type> {
    template <typename Leaf, typename Context, typename Storage>
    static auto make_guard(Context& context, const Storage&) {
        return recursion_guard<Leaf>(context);
    }

    template <typename Storage>
    static bool preserves_closure(const Storage&) {
        return false;
    }

    template <typename Context, typename Storage, typename Container>
    static auto materialize_source(Context& context, Storage& storage,
                                   Container& container) {
        using source_type = std::remove_cv_t<std::remove_reference_t<
            decltype(storage.resolve(context, container))>>;
        return make_rvalue_source<source_type>(
            std::in_place, [&](void* ptr) {
                new (ptr) source_type(storage.resolve(context, container));
            });
    }
};

template <typename Type, typename U>
struct storage_traits<
    unique, Type, U,
    std::enable_if_t<!type_traits<Type>::enabled && !std::is_reference_v<Type> &&
                     !std::is_array_v<Type> && !is_alternative_type_v<Type>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using value_types = type_list<U>;
    using lvalue_reference_types = type_list<U&>;
    using rvalue_reference_types = type_list<U&&>;
    using pointer_types = type_list<>;
    using conversion_types = type_list<>;
};

template <typename Type, typename U>
struct resolution_traits<
    unique, Type, U,
    std::enable_if_t<!type_traits<Type>::enabled && !std::is_reference_v<Type> &&
                     !std::is_array_v<Type> && !is_alternative_type_v<Type>>> {
    using value_types = type_list<std::optional<U>>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types = type_list<std::optional<U>&&>;
    using pointer_types = type_list<>;
    using conversion_types = type_list<std::optional<U>>;
};

template <typename Type, typename U>
struct storage_traits<unique, Type*, U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using value_types = type_list<std::unique_ptr<U>, std::shared_ptr<U>>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types =
        type_list<std::unique_ptr<U>&&, std::shared_ptr<U>&&>;
    using pointer_types = type_list<U*>;
    using conversion_types = type_list<std::unique_ptr<U>, std::shared_ptr<U>>;
};

template <typename T, typename U>
struct storage_traits<unique, T[], U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using rebound_unique_handle =
        wrapper_rebind_leaf_t<std::unique_ptr<T[]>, U>;
    using rebound_shared_handle =
        wrapper_rebind_leaf_t<std::shared_ptr<T[]>, U>;

    using value_types = type_list<rebound_unique_handle, rebound_shared_handle>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types =
        type_list<rebound_unique_handle&&, rebound_shared_handle&&>;
    using pointer_types = type_list<typename wrapper_rebind_leaf<T, U>::type*>;
    using conversion_types =
        type_list<rebound_unique_handle, rebound_shared_handle>;
};

template <typename T, size_t N, typename U>
struct storage_traits<unique, T[N], U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using rebound_unique_handle =
        wrapper_rebind_leaf_t<std::unique_ptr<T[]>, U>;
    using rebound_shared_handle =
        wrapper_rebind_leaf_t<std::shared_ptr<T[]>, U>;
    using rebound_row_type = typename wrapper_rebind_leaf<T, U>::type;
    using rebound_exact_type =
        typename wrapper_rebind_leaf<T[N], U>::type;

    using value_types = type_list<rebound_unique_handle, rebound_shared_handle>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types =
        type_list<rebound_unique_handle&&, rebound_shared_handle&&>;
    using pointer_types =
        type_list<rebound_row_type*, exact_lookup<rebound_exact_type>*>;
    using conversion_types =
        type_list<rebound_unique_handle, rebound_shared_handle>;
};

template <typename Array, typename Deleter, typename U>
struct storage_traits<
    unique, std::unique_ptr<Array, Deleter>, U,
    std::enable_if_t<std::is_array_v<Array> && (std::extent_v<Array, 0> == 0)>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using rebound_handle =
        wrapper_rebind_leaf_t<std::unique_ptr<Array, Deleter>, U>;
    using shared_handle =
        wrapper_rebind_leaf_t<std::shared_ptr<Array>, U>;

    using value_types = type_list<rebound_handle, shared_handle>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types = type_list<rebound_handle&&, shared_handle&&>;
    using pointer_types = type_list<>;
    using conversion_types = type_list<rebound_handle, shared_handle>;
};

template <typename T, typename Deleter, typename U>
struct storage_traits<unique, std::unique_ptr<T, Deleter>, U,
                      std::enable_if_t<!std::is_array_v<T>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using rebound_handle =
        wrapper_rebind_leaf_t<std::unique_ptr<T, Deleter>, U>;
    using inner_handle = wrapper_rebind_leaf_t<T, U>;

    using value_types = type_list<rebound_handle, std::shared_ptr<inner_handle>>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types =
        type_list<rebound_handle&&, std::shared_ptr<inner_handle>&&>;
    using pointer_types = type_list<>;
    using conversion_types =
        type_list<rebound_handle, std::shared_ptr<inner_handle>>;
};

template <typename Array, typename U>
struct storage_traits<
    unique, std::shared_ptr<Array>, U,
    std::enable_if_t<std::is_array_v<Array> && (std::extent_v<Array, 0> == 0)>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using rebound_handle = wrapper_rebind_leaf_t<std::shared_ptr<Array>, U>;

    using value_types = type_list<rebound_handle>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types = type_list<rebound_handle&&>;
    using pointer_types = type_list<>;
    using conversion_types = type_list<rebound_handle>;
};

template <typename T, typename U>
struct storage_traits<unique, std::shared_ptr<T>, U,
                      std::enable_if_t<!std::is_array_v<T>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using rebound_handle = wrapper_rebind_leaf_t<std::shared_ptr<T>, U>;

    using value_types = type_list<rebound_handle>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types = type_list<rebound_handle&&>;
    using pointer_types = type_list<>;
    using conversion_types = type_list<rebound_handle>;
};

template <typename T, typename U>
struct storage_traits<unique, std::optional<T>, U> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using value_types = type_list<std::optional<U>>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types = type_list<std::optional<U>&&>;
    using pointer_types = type_list<>;
    using conversion_types = type_list<std::optional<U>>;
};

template <typename Type, typename U>
struct storage_traits<
    unique, Type, U,
    std::enable_if_t<!type_traits<Type>::enabled && !std::is_reference_v<Type> &&
                     !std::is_array_v<Type> && is_alternative_type_v<Type>>> {
    static constexpr bool enabled = true;
    static constexpr bool is_stable = false;

    using value_types = type_list<U>;
    using lvalue_reference_types = type_list<>;
    using rvalue_reference_types = type_list<U&&>;
    using pointer_types = type_list<>;
    using conversion_types = type_list<>;
};


template <typename Type, typename U> struct conversions<unique, Type, U>
    : type_storage_traits<unique, Type, U> {};

template <typename Type, typename StoredType, typename Factory,
          typename Conversions>
class storage<unique, Type, StoredType, Factory, Conversions> : Factory {
  public:
    template <typename... Args>
    storage(Args&&... args) : Factory(std::forward<Args>(args)...) {}

    using conversions = Conversions;
    using type = Type;
    using stored_type = StoredType;
    using tag_type = unique;

    template <typename Context, typename Container>
    decltype(auto) resolve(Context& context, Container& container) {
        return Factory::template construct<Type>(context, container);
    }
};

template <typename Type, size_t N, typename StoredType, typename Factory,
          typename Conversions>
class storage<unique, Type[N], StoredType, Factory, Conversions> : Factory {
  public:
    template <typename... Args>
    storage(Args&&... args) : Factory(std::forward<Args>(args)...) {}

    using conversions = Conversions;
    using type = Type[N];
    using stored_type = StoredType;
    using tag_type = unique;

    template <typename Context, typename Container>
    decltype(auto) resolve(Context& context, Container& container) {
        return Factory::template construct<Type[N]>(context, container);
    }
};

} // export namespace silicon::di


// ==============================================================================================
// ==  umbrella  —  container entry points (container / runtime_container / static_container)
// ==============================================================================================

// --- static_container.h ---



#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

export namespace silicon::di {



struct static_container_no_dependency_diagnostics {};

template <typename StaticRegistry, bool HasParent>
struct static_container_dependency_diagnostics_base;

template <typename... Registrations>
struct static_container_dependency_diagnostics_base<
    static_registry<Registrations...>, false>
    : static_registry_dependency_diagnostics<
          typename static_registry<Registrations...>::interface_bindings,
          binding_model<Registrations>...> {};

template <typename... Registrations>
struct static_container_dependency_diagnostics_base<
    static_registry<Registrations...>, true>
    : static_container_no_dependency_diagnostics {};

template <typename StaticRegistry, typename ParentContainer = void>
class static_container_impl;

template <typename ParentContainer, typename... Registrations>
class static_container_impl<static_registry<Registrations...>, ParentContainer>
    : private static_container_dependency_diagnostics_base<
          static_registry<Registrations...>,
          !std::is_void_v<ParentContainer>> {
    using registry_type_ = static_registry<Registrations...>;
    static constexpr bool has_parent_v = !std::is_void_v<ParentContainer>;
    using graph_type_ = std::conditional_t<
        has_parent_v, graph_analysis<registry_type_, true>,
        typename static_container_graph_type<
            registry_type_, registry_type_::dependencies_are_resolved>::type>;
    using self_type = static_container_impl<registry_type_, ParentContainer>;
    using parent_container_type = ParentContainer;
    using state_type = static_storage_state<Registrations...>;
    using scope_ref = static_binding_scope_ref<state_type, Registrations...>;
    using context_type = static_context<registry_type_>;

    template <typename Request, typename LookupRequest, typename Key>
    struct diagnostic_static_binding_source {
        using selection = static_binding_t<
            typename registry_type_::template bindings<LookupRequest, Key>>;
        static constexpr bool can_resolve =
            selection::status == binding_selection_status::kFound;

        self_type& host;

        constexpr binding_selection_status status() const {
            if constexpr (has_parent_v &&
                          selection::status ==
                              binding_selection_status::kNotFound) {
                return selection::status;
            } else {
                static_assert(selection::status !=
                                  binding_selection_status::kNotFound,
                              "static_container cannot resolve an unbound "
                              "type");
            }
            static_assert(selection::status != binding_selection_status::kAmbiguous,
                          "static_container cannot resolve an ambiguously "
                          "bound type");
            return selection::status;
        }

        template <typename ResolveRequest>
        decltype(auto) resolve(context_type& context) {
            using binding = typename selection::binding_type;
            auto state = host.state_ref();
            auto resolver =
                state.template make_binding_resolver<binding>(host);
            return resolver.template resolve<Request>(context);
        }
    };

    scope_ref state_ref() { return scope_ref(static_state_); }

    template <typename Request, typename Key>
    using static_selection_t = static_binding_t<
        typename registry_type_::template bindings<Request, Key>>;

    template <typename T, bool RemoveRvalueReferences, typename Key = void>
    static constexpr bool has_static_resolve_request_v =
        static_selection_t<resolve_request_t<T, RemoveRvalueReferences>,
                           Key>::status == binding_selection_status::kFound;

    template <typename T, bool RemoveRvalueReferences, typename Key = void>
    static constexpr binding_selection_status static_resolve_status_v =
        static_selection_t<resolve_request_t<T, RemoveRvalueReferences>,
                           Key>::status;

    template <typename Collection, typename Key = void>
    static constexpr std::size_t static_collection_count_v =
        static_collection_binding_count<registry_type_, Collection, Key>();

    template <typename Collection, typename Key = void,
              typename R = as_expected_t<Collection>>
    R resolve_missing_parent_collection() {
        if (parent_) {
            if constexpr (std::is_void_v<Key>) {
                return parent_->template resolve<Collection>();
            } else {
                return parent_->template resolve<Collection>(key<Key>{});
            }
        }
        using resolve_type =
            typename collection_traits<Collection>::resolve_type;
        return std::unexpected(
            make_collection_type_not_found_exception<Collection,
                                                       resolve_type>());
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void,
              typename R = resolve_result_t<T, RemoveRvalueReferences>>
    R resolve_parent() {
        if constexpr (std::is_void_v<Key>) {
            return parent_->template resolve<T>();
        } else {
            return parent_->template resolve<T>(key<Key>{});
        }
    }

  public:
    using source_type = registry_type_;
    using static_source_type = registry_type_;
    using rtti_type = typename scope_ref::rtti_type;

    static_assert(static_source_type::valid,
                  "static_container requires a valid compile-time bindings "
                  "source");
    static_assert(graph_type_::resolvable,
                  "static_container requires a resolvable compile-time binding "
                  "graph");
    static_assert((binding_factory_is_default_constructible<
                       binding_model<Registrations>>::value &&
                   ...),
                  "static_container requires default-constructible factories");
    static_assert((binding_storage_is_default_constructible<
                       binding_model<Registrations>>::value &&
                   ...),
                  "static_container requires default-constructible storage "
                  "objects");

    static_source_type& registry() { return static_registry_; }

    const static_source_type& registry() const { return static_registry_; }

    static_container_impl() = default;

    template <typename Parent = ParentContainer,
              std::enable_if_t<!std::is_void_v<Parent>, int> = 0>
    explicit static_container_impl(Parent* parent) : parent_(parent) {}

    template <typename T, typename Key = void,
              typename R = request_result_t<T>>
    R resolve(key<Key> = {}) {
        if constexpr (!collection_traits<R>::is_collection) {
            using lookup_request_type = resolve_request_t<T, true>;
            using request_type = R;
            using selection = static_binding_t<
                typename static_source_type::template bindings<
                    lookup_request_type, Key>>;
            if constexpr (selection::status == binding_selection_status::kFound) {
                using binding = typename selection::binding_type;
                using binding_model_type =
                    typename binding::binding_model_type;
                if constexpr (binding_supports_request_v<request_type,
                                                          binding> &&
                              (std::is_reference_v<request_type> ||
                               std::is_pointer_v<request_type>) &&
                              stored_request_identity_v<
                                  request_type,
                                  typename binding_model_type::storage_type> &&
                              factory_without_dependencies_v<
                                  typename binding_model_type::factory_type>) {
                    no_dependency_context context;
                    auto state = state_ref();
                    auto resolver =
                        state.template make_binding_resolver<binding>(*this);
                    return resolver.template resolve<request_type>(context);
                }
            }
        }
        if constexpr (collection_traits<R>::is_collection) {
            if constexpr (has_parent_v &&
                          static_collection_count_v<R, Key> == 0) {
                return resolve_missing_parent_collection<R, Key>();
            } else {
                context_type context;
                return resolve<T, false, Key>(context);
            }
        } else {
            if constexpr (has_parent_v &&
                          static_resolve_status_v<T, false, Key> ==
                              binding_selection_status::kNotFound) {
                if (parent_) {
                    return resolve_parent<T, false, Key>();
                }
            }
            context_type context;
            return resolve<T, false, Key>(context);
        }
    }

    template <typename T, typename Factory = constructor<normalized_type_t<T>>,
              typename R = request_result_t<T>>
    R construct(Factory factory = Factory()) {
        using normalized_request_type = request_value_t<T>;
        using request_type = request_interface_t<T>;
        if constexpr (std::is_same_v<Factory,
                                     constructor<normalized_type_t<T>>>) {
            using selection = static_binding_t<
                typename static_source_type::template bindings<
                    binding_request_interface_t<request_type>,
                    binding_request_key_t<request_type>>>;
            using normalized_selection = static_binding_t<
                typename static_source_type::template bindings<
                    normalized_request_type, void>>;
            constexpr bool has_exact_binding =
                selection::status != binding_selection_status::kNotFound;
            constexpr bool has_normalized_binding =
                normalized_selection::status !=
                binding_selection_status::kNotFound;
            if constexpr (has_exact_binding) {
                using binding = typename selection::binding_type;
                if constexpr (binding_supports_request_v<T, binding>) {
                    return resolve<T>();
                }
            }

            if constexpr (has_exact_binding || has_normalized_binding) {
                if constexpr (::silicon::di::
                                  rvalue_request_requires_explicit_conversion_v<
                                      T>) {
                    context_type context;
                    ::silicon::di::terminate_missing_rvalue_conversion<T>(
                        has_normalized_binding, context);
                } else if constexpr (construct_normalized_request_v<T>) {
                    return construct_static_binding_value<
                        T, normalized_selection>(
                        [&]() { return resolve<normalized_request_type>(); });
                } else {
                    return resolve<T>();
                }
            } else {
                context_type context;
                auto type_guard =
                    context.template track_type<normalized_request_type>();
                return factory.template construct<R>(context, *this);
            }
        } else {
            context_type context;
            auto type_guard =
                context.template track_type<normalized_request_type>();
            return factory.template construct<R>(context, *this);
        }
    }

    template <typename Signature = void, typename Callable>
    auto invoke(Callable&& callable) {
        using callable_type =
            std::remove_cv_t<std::remove_reference_t<Callable>>;
        using dispatch_signature =
            callable_dispatch_signature_t<Signature, callable_type>;

        context_type context;
        auto type_guard = context.template track_type<callable_type>();
        return callable_invoke<dispatch_signature>::construct(
            std::forward<Callable>(callable), context, *this);
    }

    template <typename T> T construct_collection() {
        context_type context;
        auto state = state_ref();
        return state.template construct_static_collection<
            T, void, static_source_type>(*this, context);
    }

    template <typename T, typename Fn> T construct_collection(Fn&& fn) {
        context_type context;
        auto state = state_ref();
        return state.template construct_static_collection<
            T, void, static_source_type>(*this, context, std::forward<Fn>(fn));
    }

    template <typename T, typename Key> T construct_collection(key<Key>) {
        context_type context;
        auto state = state_ref();
        return state.template construct_static_collection<
            T, Key, static_source_type>(*this, context);
    }

    template <typename T, typename Fn, typename Key>
    T construct_collection(Fn&& fn, key<Key>) {
        context_type context;
        auto state = state_ref();
        return state.template construct_static_collection<
            T, Key, static_source_type>(*this, context, std::forward<Fn>(fn));
    }

    template <typename T, typename Key = void, typename Fn, typename Context>
    std::size_t append_collection(T& results, Context& context, Fn&& fn) {
        auto state = state_ref();
        return state.template append_static_collection<T, Key,
                                                       static_source_type>(
            results, *this, context, std::forward<Fn>(fn));
    }

    template <typename T, typename Key = void> std::size_t count_collection() {
        using collection_type = collection_traits<T>;
        return type_list_size_v<typename static_source_type::template bindings<
            normalized_type_t<typename collection_type::resolve_type>, Key>>;
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void,
              typename R = resolve_result_t<T, RemoveRvalueReferences>>
    R resolve(context_type& context) {
        if constexpr (collection_traits<R>::is_collection) {
            if constexpr (has_parent_v &&
                          static_collection_count_v<R, Key> == 0) {
                return resolve_missing_parent_collection<R, Key>();
            } else {
                auto state = state_ref();
                return state.template construct_static_collection<
                    R, Key, static_source_type>(*this, context);
            }
        } else {
            using lookup_request_type =
                resolve_request_t<T, RemoveRvalueReferences>;
            using request_type = R;
            if constexpr (has_parent_v) {
                if constexpr (static_resolve_status_v<
                                  T, RemoveRvalueReferences, Key> ==
                              binding_selection_status::kNotFound) {
                    if (parent_) {
                        return resolve_parent<T, RemoveRvalueReferences, Key>();
                    }
                }
            }
            diagnostic_static_binding_source<request_type, lookup_request_type,
                                             Key>
                source{*this};
            missing_binding_source<lookup_request_type> missing;
            auto sources = make_one_binding_source(source, missing);
            return resolve_from_binding_sources<T, request_type>(context,
                                                                 sources);
        }
    }

    template <typename T, bool RemoveRvalueReferences, typename Key,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve(context_type& context, key<Key>) {
        return resolve<T, RemoveRvalueReferences, Key>(context);
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache,
              typename Key,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve(context_type& context, key<Key>) {
        (void)CheckCache;
        return resolve<T, RemoveRvalueReferences, Key>(context);
    }

  private:
    static_source_type static_registry_;
    state_type static_state_;
    parent_container_type* parent_ = nullptr;
};



template <typename StaticSource, typename ParentContainer>
class static_container
    : public static_container_impl<
          static_bindings_source_t<StaticSource>, ParentContainer> {
    using base_type = static_container_impl<
        static_bindings_source_t<StaticSource>, ParentContainer>;

  public:
    using base_type::base_type;
};

} // export namespace silicon::di

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// --- container.h ---



#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif

export namespace silicon::di {

template <typename... Ts>
inline constexpr bool container_dependent_false_v = false;

template <typename T> struct is_static_context_argument : std::false_type {};

template <typename StaticRegistry, bool RuntimeDependencies>
struct is_static_context_argument<
    basic_static_context<StaticRegistry, RuntimeDependencies>> : std::true_type {
};

template <typename T>
inline constexpr bool is_static_context_argument_v =
    is_static_context_argument<
        std::remove_cv_t<std::remove_reference_t<T>>>::value;

template <typename Parent, typename T, bool RemoveRvalueReferences,
          bool CheckCache, typename Key, typename = void>
struct parent_context_resolve_supported : std::false_type {};

template <typename Parent, typename T, bool RemoveRvalueReferences,
          bool CheckCache>
struct parent_context_resolve_supported<
    Parent, T, RemoveRvalueReferences, CheckCache, void,
    std::void_t<decltype(std::declval<Parent&>().template resolve<
                         T, RemoveRvalueReferences, CheckCache>(
        std::declval<runtime_context&>()))>> : std::true_type {};

template <typename Parent, typename T, bool RemoveRvalueReferences,
          bool CheckCache, typename Key>
struct parent_context_resolve_supported<
    Parent, T, RemoveRvalueReferences, CheckCache, Key,
    std::void_t<decltype(std::declval<Parent&>().template resolve<
                         T, RemoveRvalueReferences, CheckCache>(
        std::declval<runtime_context&>(), key<Key>{}))>> : std::true_type {};

template <typename Parent, typename T, bool RemoveRvalueReferences,
          bool CheckCache, typename Key>
inline constexpr bool parent_context_resolve_supported_v =
    parent_context_resolve_supported<Parent, T, RemoveRvalueReferences,
                                     CheckCache, Key>::value;


template <typename ParentContainer, typename... Registrations>
class container_with_static_bindings<static_registry<Registrations...>,
                                             ParentContainer>
    : public runtime_registration_api<
          container_with_static_bindings<
              static_registry<Registrations...>, ParentContainer>> {
    // 显式限定：类属 detail 命名空间，非限定 friend 指向外层 silicon::di 的类型
    // 属 Microsoft 扩展（-Werror,-Wmicrosoft-unqualified-friend）。
    friend class silicon::di::runtime_context;

    using static_registry_type_ = static_registry<Registrations...>;
    using self_type =
        container_with_static_bindings<static_registry_type_,
                                               ParentContainer>;
    using runtime_base =
        runtime_registry<dynamic_container_traits,
                         typename dynamic_container_traits::allocator_type,
                         self_type, self_type>;
    using static_state = static_storage_state<Registrations...>;
    using static_resolution_ref =
        static_binding_scope_ref<static_state, Registrations...>;
    using binding_resolution_ref =
        binding_scope_ref<static_state, Registrations...>;
    using static_context_type = static_context<static_registry_type_>;
    static constexpr bool has_parent_v = !std::is_void_v<ParentContainer>;
    using parent_container_type = ParentContainer;

    template <typename T, typename Key>
    using static_selection_t = static_binding_t<
        typename static_registry_type_::template bindings<T, Key>>;
    template <typename Key>
    using collection_key_t =
        std::conditional_t<std::is_void_v<Key>, none_t, key<Key>>;

    template <typename Key> static collection_key_t<Key> collection_key() {
        return {};
    }

    template <typename T>
    static constexpr bool runtime_auto_constructible_v =
        std::is_same_v<request_value_t<T>, std::decay_t<T>> &&
        (!std::is_reference_v<T> ||
         (std::is_lvalue_reference_v<T> &&
          std::is_const_v<std::remove_reference_t<T>> &&
          is_auto_constructible<std::decay_t<T>>::value));

    template <typename T, bool CheckCache, typename IdType>
    struct selected_runtime_binding {
        runtime_base& registry;
        IdType& id;

        decltype(auto) select() {
            return registry.template runtime_source_select<T>(
                std::forward<IdType>(id));
        }

        template <typename Request, typename Selection>
        decltype(auto) resolve(runtime_context& context, Selection selection) {
            return registry.template runtime_source_resolve<T, CheckCache>(
                selection, context, std::forward<IdType>(id));
        }
    };

    template <typename T, bool RemoveRvalueReferences, bool MayAutoConstruct,
              typename IdType>
    struct missing_runtime_binding {
        self_type& self;
        IdType& id;

        template <typename Request>
        request_interface_t<Request> resolve(runtime_context& context) {
            return self.template resolve_missing_runtime<
                T, RemoveRvalueReferences, MayAutoConstruct, IdType,
                request_interface_t<Request>>(context,
                                              std::forward<IdType>(id));
        }
    };

    template <typename T, bool RemoveRvalueReferences, typename Key,
              typename Request>
    struct runtime_binding_candidate {
        static constexpr bool can_resolve = true;

        self_type& self;

        binding_selection_status status() const {
            return self.runtime_registry_.template binding_status<Request,
                                                                  Key>();
        }

        template <typename ResolveRequest>
        decltype(auto) resolve(runtime_context& context) {
            return self.template resolve_runtime_only<
                T, RemoveRvalueReferences, runtime_auto_constructible_v<T>>(
                context, collection_key<Key>());
        }
    };

    template <typename Request, typename Key>
    struct static_binding_candidate {
        using selection = static_binding_t<
            typename static_registry_type_::template bindings<Request, Key>>;
        static constexpr bool can_resolve =
            selection::status == binding_selection_status::kFound;

        self_type& self;

        constexpr binding_selection_status status() const {
            return selection::status;
        }

        template <typename ResolveRequest>
        decltype(auto) resolve(runtime_context& context) {
            return self.template resolve_binding_selection<Request, Key>(
                context);
        }
    };

    template <typename Request, typename Key = void>
    bool has_runtime_binding() {
        if (!runtime_registry_.has_runtime_registrations()) {
            return false;
        }
        return runtime_registry_.template binding_status<Request, Key>() !=
               binding_selection_status::kNotFound;
    }

    template <typename T, typename Key = void> bool has_runtime_collection() {
        if (!runtime_registry_.has_runtime_registrations()) {
            return false;
        }
        return runtime_registry_.template count_runtime_collection<T>(
                   collection_key<Key>()) != 0;
    }

    bool has_runtime_registrations() const {
        return runtime_registry_.has_runtime_registrations();
    }

    template <typename Request, typename Key = void>
    static constexpr bool has_static_binding_v =
        static_selection_t<Request, Key>::status ==
        binding_selection_status::kFound;

    template <typename Request, typename Key = void,
              bool Selected = has_static_binding_v<Request, Key>>
    struct static_binding_satisfies_request : std::false_type {};

    template <typename Request, typename Key>
    struct static_binding_satisfies_request<Request, Key, true>
        : std::bool_constant<
              (has_parent_v ||
               static_binding_resolvable_v<
                   typename static_selection_t<Request, Key>::binding_type,
                   static_registry_type_>) &&
              binding_supports_request_v<
                  request_interface_t<Request>,
                  typename static_selection_t<Request, Key>::binding_type>> {};

    template <typename Request, typename Key = void>
    static constexpr bool static_binding_satisfies_request_v =
        static_binding_satisfies_request<Request, Key>::value;

    template <typename Request, typename Key = void>
    bool select_static_resolve() {
        if constexpr (!static_binding_satisfies_request_v<Request, Key>) {
            return false;
        } else if (!has_runtime_registrations()) {
            return true;
        } else {
            return !has_runtime_binding<Request, Key>();
        }
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void>
    static constexpr bool has_static_resolve_request_v =
        static_binding_satisfies_request_v<
            resolve_request_t<T, RemoveRvalueReferences>, Key>;

    template <typename T, bool RemoveRvalueReferences, typename Key = void>
    static constexpr binding_selection_status static_resolve_status_v =
        static_selection_t<resolve_request_t<T, RemoveRvalueReferences>,
                           Key>::status;

    template <typename Collection, typename Key = void>
    bool select_static_collection() {
        if constexpr (!has_static_collection_v<Collection, Key>) {
            return false;
        } else if (!has_runtime_registrations()) {
            return true;
        } else {
            return !has_runtime_collection<Collection, Key>();
        }
    }

    template <typename T>
    static constexpr bool has_static_construct_request_v = [] {
        using request_type = request_interface_t<T>;
        using normalized_request_type = request_value_t<T>;
        constexpr bool has_exact_static_binding =
            static_binding_satisfies_request_v<request_type>;
        constexpr bool has_normalized_static_binding =
            static_selection_t<normalized_request_type, void>::status ==
                binding_selection_status::kFound &&
            static_binding_resolvable_v<
                typename static_selection_t<normalized_request_type,
                                            void>::binding_type,
                static_registry_type_>;

        return has_exact_static_binding || (has_normalized_static_binding &&
                                            construct_normalized_request_v<T>);
    }();

    template <typename T> bool select_static_construct() {
        using request_type = request_interface_t<T>;
        using normalized_request_type = request_value_t<T>;

        if constexpr (!has_static_construct_request_v<T>) {
            return false;
        } else if (!has_runtime_registrations()) {
            return true;
        } else {
            return !has_runtime_binding<request_type>() &&
                   !has_runtime_binding<normalized_request_type>();
        }
    }

    template <typename T, typename Key = void>
    bool select_static_collection_construct() {
        constexpr bool has_static_collection_bindings =
            static_collection_binding_count<static_registry_type_, T,
                                                    Key>() != 0;
        if constexpr (!has_static_collection_bindings) {
            return false;
        } else if (!has_runtime_registrations()) {
            return true;
        } else {
            return !has_runtime_collection<T, Key>();
        }
    }

    template <typename Collection, typename Key = void>
    static constexpr bool has_static_collection_v =
        static_collection_binding_count<static_registry_type_,
                                                Collection, Key>() != 0 &&
        static_bindings_resolvable_v<
            typename static_registry_type_::template bindings<
                normalized_type_t<
                    typename collection_traits<Collection>::resolve_type>,
                Key>,
            static_registry_type_>;

    template <typename T, typename Key, typename Fn, typename Context>
    T construct_static_collection(Context& context, Fn&& fn, key<Key>) {
        static_resolution_ref static_state_ref(
            static_state_);
        return static_state_ref.template construct_static_collection<
            T, Key, static_registry_type_>(*this, context,
                                           std::forward<Fn>(fn));
    }

    template <typename T, typename Fn, typename Context>
    T construct_static_collection(Context& context, Fn&& fn, none_t) {
        static_resolution_ref static_state_ref(
            static_state_);
        return static_state_ref.template construct_static_collection<
            T, void, static_registry_type_>(*this, context,
                                            std::forward<Fn>(fn));
    }

    template <typename T, typename Key, typename Context>
    T construct_static_collection(Context& context, key<Key>) {
        static_resolution_ref static_state_ref(
            static_state_);
        return static_state_ref.template construct_static_collection<
            T, Key, static_registry_type_>(*this, context);
    }

    template <typename T, typename Context>
    T construct_static_collection(Context& context, none_t) {
        static_resolution_ref static_state_ref(
            static_state_);
        return static_state_ref.template construct_static_collection<
            T, void, static_registry_type_>(*this, context);
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void,
              typename R = request_result_t<T>,
              std::enable_if_t<std::is_rvalue_reference_v<T>, int> = 0>
    SILICON_DI_ALWAYS_INLINE R resolve_static() {
        static_context_type static_context;
        return resolve_static<T, RemoveRvalueReferences, Key>(static_context);
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void,
              std::enable_if_t<!std::is_rvalue_reference_v<T>, int> = 0>
    SILICON_DI_ALWAYS_INLINE decltype(auto) resolve_static() {
        static_context_type static_context;
        return resolve_static<T, RemoveRvalueReferences, Key>(static_context);
    }

    template <typename T, typename R = request_result_t<T>>
    SILICON_DI_ALWAYS_INLINE R construct_static() {
        using request_type = request_interface_t<T>;
        using normalized_request_type = request_value_t<T>;
        constexpr bool has_exact_static_binding =
            static_binding_satisfies_request_v<request_type>;
        constexpr bool has_normalized_static_binding =
            static_binding_satisfies_request_v<normalized_request_type>;
        if constexpr (has_exact_static_binding) {
            using binding =
                typename static_selection_t<request_type, void>::binding_type;
            if constexpr (binding_supports_request_v<T, binding>) {
                static_context_type static_context;
                return resolve_static<T, false>(static_context);
            }
        }

        if constexpr (has_normalized_static_binding &&
                      construct_normalized_request_v<T>) {
            using normalized_selection =
                static_selection_t<normalized_request_type, void>;
            return construct_static_binding_value<T,
                                                          normalized_selection>(
                [&]() {
                    return resolve_static<normalized_request_type, false>();
                });
        } else {
            static_context_type static_context;
            return resolve_static<T, false>(static_context);
        }
    }

    template <
        typename T, typename Fn,
        std::enable_if_t<!is_static_context_argument_v<Fn>, int> = 0>
    T construct_static_collection(Fn&& fn, none_t) {
        static_context_type static_context;
        return construct_static_collection<T>(static_context,
                                              std::forward<Fn>(fn), none_t{});
    }

    template <typename T> T construct_static_collection(none_t) {
        static_context_type static_context;
        return construct_static_collection<T>(static_context, none_t{});
    }

    template <
        typename T, typename Fn, typename Key,
        std::enable_if_t<!is_static_context_argument_v<Fn>, int> = 0>
    T construct_static_collection(Fn&& fn, key<Key>) {
        static_context_type static_context;
        return construct_static_collection<T>(static_context,
                                              std::forward<Fn>(fn), key<Key>{});
    }

    template <typename T, typename Key>
    T construct_static_collection(key<Key>) {
        static_context_type static_context;
        return construct_static_collection<T>(static_context, key<Key>{});
    }

    template <typename T, typename Key, typename Fn>
    T construct_collection_impl(runtime_context& context, Fn&& fn) {
        using collection_type = collection_traits<T>;
        using resolve_type = typename collection_type::resolve_type;
        binding_resolution_ref static_state_ref(
            static_state_);

        constexpr std::size_t static_count =
            type_list_size_v<typename static_registry_type_::template bindings<
                normalized_type_t<resolve_type>, Key>>;
        return construct_binding_collection<T>(
            [&] {
                return runtime_registry_.template count_runtime_collection<T>(
                    collection_key<Key>());
            },
            [&] { return static_count; },
            [&](auto& results, auto&& append) {
                runtime_registry_.append_runtime_collection(
                    results, context, std::forward<decltype(append)>(append),
                    collection_key<Key>());
            },
            [&](auto& results, auto&& append) {
                static_state_ref.template append_static_collection<
                    T, Key, static_registry_type_>(
                    results, *this, context,
                    std::forward<decltype(append)>(append));
            },
            std::forward<Fn>(fn));
    }

    template <typename T, typename Key, typename Fn>
    T construct_collection(runtime_context& context, Fn&& fn, key<Key>) {
        return construct_collection_impl<T, Key>(context, std::forward<Fn>(fn));
    }

    template <typename T, typename Fn>
    T construct_collection(runtime_context& context, Fn&& fn, none_t) {
        return construct_collection_impl<T, void>(context,
                                                  std::forward<Fn>(fn));
    }

    template <typename LookupRequest, typename Request, typename Key,
              typename Context>
    SILICON_DI_ALWAYS_INLINE resolve_expected_t<Request>
    resolve_static_selection(Context& context) {
        using selection = static_selection_t<LookupRequest, Key>;
        using binding = typename selection::binding_type;
        if constexpr (selection::status !=
                      binding_selection_status::kFound) {
            if constexpr (selection::status ==
                          binding_selection_status::kAmbiguous) {
                return std::unexpected(
                    make_type_ambiguous_exception<LookupRequest>(context));
            } else {
                return std::unexpected(
                    make_type_not_found_exception<LookupRequest>(context));
            }
        } else {
            static_resolution_ref static_state_ref(
                static_state_);
            auto resolver =
                static_state_ref.template make_binding_resolver<binding>(
                    *this);
            return resolver.template resolve<Request>(context);
        }
    }

    template <typename Request, typename Key, typename Context>
    resolve_expected_t<Request> resolve_binding_selection(Context& context) {
        using selection = static_selection_t<Request, Key>;
        using binding = typename selection::binding_type;
        if constexpr (selection::status !=
                      binding_selection_status::kFound) {
            if constexpr (selection::status ==
                          binding_selection_status::kAmbiguous) {
                return std::unexpected(
                    make_type_ambiguous_exception<Request>(context));
            } else {
                return std::unexpected(
                    make_type_not_found_exception<Request>(context));
            }
        } else {
            binding_resolution_ref static_state_ref(
                static_state_);
            auto resolver =
                static_state_ref.template make_binding_resolver<binding>(
                    *this);
            return resolver.template resolve<Request>(context);
        }
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void,
              typename Context,
              typename R = resolve_result_t<T, RemoveRvalueReferences>>
    SILICON_DI_ALWAYS_INLINE R resolve_static(Context& context) {
        if constexpr (collection_traits<R>::is_collection) {
            return construct_static_collection<R>(context,
                                                  collection_key<Key>());
        } else {
            using lookup_request_type =
                resolve_request_t<T, RemoveRvalueReferences>;
            using request_type = R;
            return resolve_static_selection<lookup_request_type, request_type,
                                            Key>(context);
        }
    }

    template <typename T, typename Fn, typename IdType>
#if defined(__GNUC__) && !defined(__clang__)
    __attribute__((noinline))
#endif
    T construct_collection_runtime_context(Fn&& fn, IdType id) {
        runtime_context context;
        return construct_collection<T>(context, std::forward<Fn>(fn), id);
    }

    template <typename T, bool RemoveRvalueReferences, bool MayAutoConstruct,
              typename IdType,
              typename R = resolve_expected_result_t<T, RemoveRvalueReferences>>
    R resolve_missing_runtime(runtime_context& context, IdType&& id) {
        using Type = normalized_type_t<T>;
        (void)id;

        if constexpr (MayAutoConstruct && is_typed_key_v<IdType> &&
                      collection_traits<R>::is_collection) {
            return construct_collection_runtime_context<R>(
                binding_collection_append{}, std::decay_t<IdType>{});
        } else if constexpr (MayAutoConstruct &&
                             is_auto_constructible<std::decay_t<T>>::value) {
            if constexpr (constructor<Type>::kind ==
                          constructor_kind::kConcrete) {
                static_assert(is_complete<Type>::value,
                              "auto-construction requires a complete type");
                using type_detection = automatic;
                return context.template construct_temporary<
                    request_interface_t<T>, type_detection>(*this);
            } else if constexpr (is_none_v<std::decay_t<IdType>>) {
                return std::unexpected(
                    make_type_not_found_exception<T>(context));
            } else {
                return std::unexpected(make_type_not_found_exception<
                    T, std::decay_t<IdType>>(context));
            }
        } else if constexpr (is_none_v<std::decay_t<IdType>>) {
            return std::unexpected(
                make_type_not_found_exception<T>(context));
        } else {
            return std::unexpected(make_type_not_found_exception<T,
                                                        std::decay_t<IdType>>(
                context));
        }
    }

    template <typename T, bool RemoveRvalueReferences, bool MayAutoConstruct,
              bool CheckCache = true, typename IdType = none_t,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve_runtime_only(runtime_context& context, IdType&& id = IdType()) {
        using Type = normalized_type_t<T>;
        static_assert(!std::is_const_v<Type>);

        selected_runtime_binding<T, CheckCache, IdType> selected{
            runtime_registry_, id};
        missing_runtime_binding<T, RemoveRvalueReferences, MayAutoConstruct,
                                IdType>
            missing{*this, id};
        auto sources = make_selected_binding_sources(selected, missing);
        return resolve_from_binding_sources<T, R>(context, sources);
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void,
              typename R = resolve_result_t<T, RemoveRvalueReferences>>
    R resolve_parent(none_t = {}) {
        if constexpr (std::is_void_v<Key>) {
            return parent_->template resolve<T>();
        } else {
            return parent_->template resolve<T>(key<Key>{});
        }
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache,
              typename Key = void,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve_parent(runtime_context& context) {
        if constexpr (parent_context_resolve_supported_v<
                          parent_container_type, T, RemoveRvalueReferences,
                          CheckCache, Key>) {
            if constexpr (std::is_void_v<Key>) {
                return parent_->template resolve<T, RemoveRvalueReferences,
                                                 CheckCache>(context);
            } else {
                return parent_->template resolve<T, RemoveRvalueReferences,
                                                 CheckCache>(context, key<Key>{});
            }
        } else {
            return resolve_parent<T, RemoveRvalueReferences, Key>();
        }
    }

    template <typename T, typename Factory = constructor<normalized_type_t<T>>,
              typename R = request_result_t<T>>
    R construct_runtime_only(Factory factory = Factory()) {
        runtime_context context;
        if constexpr (std::is_same_v<Factory,
                                     constructor<normalized_type_t<T>>>) {
            if (runtime_registry_.template binding_status<T>() !=
                binding_selection_status::kNotFound) {
                if constexpr (::silicon::di::
                                  rvalue_request_requires_explicit_conversion_v<
                                      T>) {
                    return resolve_runtime_only<
                        T, false, runtime_auto_constructible_v<T>>(
                        context, none_t{});
                } else if constexpr (construct_normalized_request_v<T>) {
                    return ::silicon::di::construct_request_or_wrap_normalized<T>(
                        [&]() {
                            return resolve_runtime_only<
                                T, false, runtime_auto_constructible_v<T>>(
                                context, none_t{});
                        },
                        [&]() {
                            return resolve_runtime_only<
                                normalized_type_t<T>, false,
                                runtime_auto_constructible_v<
                                    normalized_type_t<T>>>(context, none_t{});
                        });
                } else {
                    return resolve_runtime_only<
                        T, false, runtime_auto_constructible_v<T>>(
                        context, none_t{});
                }
            } else if (runtime_registry_
                           .template binding_status<normalized_type_t<T>>() !=
                       binding_selection_status::kNotFound) {
                if constexpr (::silicon::di::
                                  rvalue_request_requires_explicit_conversion_v<
                                      T>) {
                    ::silicon::di::terminate_missing_rvalue_conversion<T>(true, context);
                } else if constexpr (construct_normalized_request_v<T>) {
                    return type_traits<std::decay_t<T>>::make(
                        resolve_runtime_only<
                            normalized_type_t<T>, false,
                            runtime_auto_constructible_v<normalized_type_t<T>>>(
                            context, none_t{}));
                } else {
                    return resolve_runtime_only<
                        T, false, runtime_auto_constructible_v<T>>(
                        context, none_t{});
                }
            }
        }

        if constexpr (construct_factory_request_v<T>) {
            auto type_guard =
                context.template track_type<normalized_type_t<T>>();
            return factory.template construct<R>(context, *this);
        } else if constexpr (::silicon::di::
                                 rvalue_request_requires_explicit_conversion_v<
                                     T>) {
            ::silicon::di::terminate_missing_rvalue_conversion<T>(false, context);
        } else {
            return resolve_runtime_only<T, false, runtime_auto_constructible_v<T>>(
                context, none_t{});
        }
    }

  public:
    using runtime_registry_type = runtime_base;
    using static_registry_type = static_registry_type_;
    using container_traits_type = typename runtime_base::container_traits_type;
    using allocator_type = typename runtime_base::allocator_type;
    using rtti_type = typename runtime_base::rtti_type;
    using static_source_type = static_registry_type;

    static_assert(static_source_type::valid,
                  "container requires a valid compile-time bindings source");
    static_assert(graph_analysis<static_source_type, true>::resolvable,
                  "container requires a resolvable compile-time binding graph");
    static_assert(
        (binding_factory_is_default_constructible<
             binding_model<Registrations>>::value &&
         ...),
        "container requires default-constructible compile-time factories");
    static_assert((binding_storage_is_default_constructible<
                       binding_model<Registrations>>::value &&
                   ...),
                  "container requires default-constructible compile-time "
                  "storage objects");

    container_with_static_bindings() : runtime_registry_(nullptr) {}

    explicit container_with_static_bindings(allocator_type alloc)
        : runtime_registry_(nullptr, alloc) {}

    template <typename Parent = ParentContainer,
              std::enable_if_t<!std::is_void_v<Parent>, int> = 0>
    explicit container_with_static_bindings(Parent* parent,
                                            allocator_type alloc =
                                                allocator_type())
        : runtime_registry_(nullptr, alloc), parent_(parent) {}

    container_with_static_bindings(const self_type& other)
        : runtime_registry_(other.runtime_registry_),
          static_registry_(other.static_registry_),
          static_state_(other.static_state_),
          parent_(other.parent_) {
    }

    container_with_static_bindings(self_type&& other)
        : runtime_registry_(std::move(other.runtime_registry_)),
          static_registry_(std::move(other.static_registry_)),
          static_state_(std::move(other.static_state_)),
          parent_(other.parent_) {
    }

    self_type& operator=(const self_type& other) {
        if (this != &other) {
            runtime_registry_ = other.runtime_registry_;
            static_registry_ = other.static_registry_;
            static_state_ = other.static_state_;
            parent_ = other.parent_;
        }
        return *this;
    }

    self_type& operator=(self_type&& other) {
        if (this != &other) {
            runtime_registry_ = std::move(other.runtime_registry_);
            static_registry_ = std::move(other.static_registry_);
            static_state_ = std::move(other.static_state_);
            parent_ = other.parent_;
        }
        return *this;
    }

    self_type& registry() { return *this; }

    const self_type& registry() const { return *this; }

    self_type& container() { return *this; }

    const self_type& container() const { return *this; }

    template <typename T, typename IdType = none_t,
              typename R = request_result_t<T>>
    SILICON_DI_ALWAYS_INLINE R resolve(IdType&& id = IdType()) {
        if constexpr (is_typed_key_v<IdType>) {
            using key_type = typename std::decay_t<IdType>::type;
            if constexpr (collection_traits<R>::is_collection) {
                if constexpr (has_static_collection_v<R, key_type>) {
                    if (select_static_collection<R, key_type>()) {
                        return resolve_static<T, false, key_type>();
                    }
                }
                if constexpr (has_parent_v) {
                    if (parent_ && count_collection<R, key_type>() == 0) {
                        return resolve_parent<T, false, key_type>();
                    }
                }
            } else {
                if (select_static_resolve<T, key_type>()) {
                    return resolve_static<T, false, key_type>();
                }
                if constexpr (has_parent_v) {
                    if constexpr (static_resolve_status_v<
                                      T, false, key_type> ==
                                  binding_selection_status::kNotFound) {
                        if (parent_ &&
                            resolve_binding_status<T, false, key_type>() ==
                                binding_selection_status::kNotFound) {
                            return resolve_parent<T, false, key_type>();
                        }
                    }
                }
            }
            runtime_context context;
            return resolve<T, false, true, key_type>(context);
        } else if constexpr (!is_none_v<std::decay_t<IdType>>) {
            if constexpr (has_parent_v) {
                if constexpr (collection_traits<R>::is_collection) {
                    if (parent_ &&
                        runtime_registry_
                                .template count_runtime_collection<R>(id) ==
                            0) {
                        return parent_->template resolve<T>(
                            std::forward<IdType>(id));
                    }
                } else {
                    if (parent_ &&
                        runtime_registry_.template binding_status_for_id<T>(
                            id) ==
                            binding_selection_status::kNotFound) {
                        return parent_->template resolve<T>(
                            std::forward<IdType>(id));
                    }
                }
            }
            runtime_context context;
            return resolve_runtime_only<T, false, runtime_auto_constructible_v<T>>(
                context, std::forward<IdType>(id));
        } else {
            if constexpr (collection_traits<R>::is_collection) {
                if constexpr (has_static_collection_v<R>) {
                    if (select_static_collection<R>()) {
                        return resolve_static<T, false>();
                    }
                }
                if constexpr (has_parent_v) {
                    if (parent_ && count_collection<R>() == 0) {
                        return resolve_parent<T, false>();
                    }
                }
            } else {
                if (select_static_resolve<T>()) {
                    return resolve_static<T, false>();
                }
                if constexpr (has_parent_v) {
                    if constexpr (static_resolve_status_v<
                                      T, false> ==
                                  binding_selection_status::kNotFound) {
                        if (parent_ &&
                            resolve_binding_status<T, false>() ==
                                binding_selection_status::kNotFound) {
                            return resolve_parent<T, false>();
                        }
                    }
                }
            }
            runtime_context context;
            return resolve<T, false>(context);
        }
    }

    template <typename T, typename Factory = constructor<normalized_type_t<T>>,
              typename R = request_result_t<T>>
    SILICON_DI_ALWAYS_INLINE R construct(Factory factory = Factory()) {
        using request_type = request_interface_t<T>;
        using normalized_request_type = request_value_t<T>;
        if constexpr (std::is_same_v<Factory,
                                     constructor<normalized_type_t<T>>>) {
            if constexpr (has_static_construct_request_v<T>) {
                if (!has_runtime_registrations()) {
                    return construct_static<T>();
                }
            }
            if constexpr (::silicon::di::
                              rvalue_request_requires_explicit_conversion_v<
                                  T>) {
                constexpr bool has_static_normalized_binding =
                    static_selection_t<normalized_request_type, void>::status !=
                        binding_selection_status::kNotFound &&
                    static_binding_resolvable_v<
                        typename static_selection_t<normalized_request_type,
                                                    void>::binding_type,
                        static_registry_type_>;

                if constexpr (has_static_construct_request_v<T>) {
                    if (binding_status<request_type>() !=
                            binding_selection_status::kNotFound &&
                        select_static_construct<T>()) {
                        return construct_static<T>();
                    }
                }

                if (has_runtime_binding<request_type>() ||
                    has_runtime_binding<normalized_request_type>()) {
                    return construct_runtime_only<T>(std::move(factory));
                }

                if constexpr (has_static_normalized_binding) {
                    ::silicon::di::terminate_missing_rvalue_conversion<T>(true);
                }

                return construct_runtime_only<T>(std::move(factory));
            } else if constexpr (has_static_construct_request_v<T>) {
                if (binding_status<request_type>() !=
                        binding_selection_status::kNotFound &&
                    select_static_construct<T>()) {
                    return construct_static<T>();
                }
            } else {
                return construct_runtime_only<T>(std::move(factory));
            }
        }
        runtime_context context;
        if constexpr (std::is_same_v<Factory,
                                     constructor<normalized_type_t<T>>>) {
            if (binding_status<T>() !=
                binding_selection_status::kNotFound) {
                if constexpr (construct_normalized_request_v<T>) {
                    return ::silicon::di::construct_request_or_wrap_normalized<T>(
                        [&]() { return resolve<T, false>(context); },
                        [&]() {
                            return resolve<normalized_type_t<T>, false>(
                                context);
                        });
                } else {
                    return resolve<T, false>(context);
                }
            } else if (binding_status<normalized_type_t<T>>() !=
                       binding_selection_status::kNotFound) {
                if constexpr (construct_normalized_request_v<T>) {
                    return type_traits<std::decay_t<T>>::make(
                        resolve<normalized_type_t<T>, false>(context));
                } else {
                    return resolve<T, false>(context);
                }
            }
        }

        if constexpr (construct_factory_request_v<T>) {
            auto type_guard =
                context.template track_type<normalized_type_t<T>>();
            return factory.template construct<R>(context, *this);
        } else {
            return resolve<T, false>(context);
        }
    }

    template <typename T> T construct_collection() {
        if (select_static_collection_construct<T>()) {
            return construct_static_collection<T>(none_t{});
        }

        return construct_collection_runtime_context<T>(
            binding_collection_append{}, none_t{});
    }

    template <typename T, typename Fn> T construct_collection(Fn&& fn) {
        if (select_static_collection_construct<T>()) {
            return construct_static_collection<T>(std::forward<Fn>(fn),
                                                  none_t{});
        }

        return construct_collection_runtime_context<T>(
            std::forward<Fn>(fn), none_t{});
    }

    template <typename T, typename Key> T construct_collection(key<Key>) {
        if (select_static_collection_construct<T, Key>()) {
            return construct_static_collection<T>(key<Key>{});
        }

        return construct_collection_runtime_context<T>(
            binding_collection_append{}, key<Key>{});
    }

    template <typename T, typename Fn, typename Key>
    T construct_collection(Fn&& fn, key<Key>) {
        if (select_static_collection_construct<T, Key>()) {
            return construct_static_collection<T>(std::forward<Fn>(fn),
                                                  key<Key>{});
        }

        return construct_collection_runtime_context<T>(
            std::forward<Fn>(fn), key<Key>{});
    }

    template <typename Signature = void, typename Callable>
    auto invoke(Callable&& callable) {
        using callable_type =
            std::remove_cv_t<std::remove_reference_t<Callable>>;
        using dispatch_signature =
            callable_dispatch_signature_t<Signature, callable_type>;

        runtime_context context;
        auto type_guard = context.template track_type<callable_type>();
        return callable_invoke<dispatch_signature>::construct(
            std::forward<Callable>(callable), context, *this);
    }

    template <typename Request, typename Key = void>
    binding_selection_status binding_status() {
        using request_type = request_interface_t<Request>;
        using static_selection = static_selection_t<request_type, Key>;
        const auto runtime_status =
            runtime_registry_.template binding_status<request_type, Key>();
        return resolve_binding_status<static_selection::status>(
            runtime_status,
            binding_resolution_policy::kAmbiguousOnConflict);
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void>
    binding_selection_status resolve_binding_status() {
        using request_type = resolve_request_t<T, RemoveRvalueReferences>;
        const auto runtime_status =
            runtime_registry_.template binding_status<request_type, Key>();
        return resolve_binding_status<
            static_resolve_status_v<T, RemoveRvalueReferences, Key>>(
            runtime_status,
            binding_resolution_policy::kAmbiguousOnConflict);
    }

    template <typename T, typename Key = void, typename Fn>
    std::size_t append_collection(T& results, runtime_context& context,
                                  Fn&& fn) {
        return append_collection_impl<T, Key>(results, context, context,
                                              std::forward<Fn>(fn));
    }

    template <typename T, typename Key = void, typename Fn, typename Context,
              std::enable_if_t<!std::is_same_v<std::remove_reference_t<Context>,
                                               runtime_context>,
                               int> = 0>
    std::size_t append_collection(T& results, Context& context, Fn&& fn) {
        runtime_context runtime_append_context;
        return append_collection_impl<T, Key>(
            results, runtime_append_context, context, std::forward<Fn>(fn));
    }

    template <typename T, typename Key = void, typename Fn, typename Context>
    std::size_t append_collection_impl(T& results,
                                       runtime_context& runtime_context,
                                       Context& static_context, Fn&& fn) {
        binding_resolution_ref static_state_ref(
            static_state_);
        return append_binding_collection(
            results,
            [&](auto& collection, auto&& append) {
                return runtime_registry_.append_runtime_collection(
                    collection, runtime_context,
                    std::forward<decltype(append)>(append),
                    collection_key<Key>());
            },
            [&](auto& collection, auto&& append) {
                return static_state_ref.template append_static_collection<
                    T, Key, static_registry_type_>(
                    collection, *this, static_context,
                    std::forward<decltype(append)>(append));
            },
            std::forward<Fn>(fn));
    }

    template <typename T, typename Key = void> std::size_t count_collection() {
        return count_binding_collection<T>(
            [&] {
                return runtime_registry_.template count_runtime_collection<T>(
                    collection_key<Key>());
            },
            static_collection_binding_count<static_registry_type_, T,
                                                    Key>());
    }

    template <typename T, bool RemoveRvalueReferences, typename Key = void,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve_request(runtime_context& context) {
        return resolve<T, RemoveRvalueReferences, true>(context,
                                                        collection_key<Key>());
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache = true,
              typename R = resolve_result_t<T, RemoveRvalueReferences>>
    R resolve(static_context_type& context, none_t) {
        if constexpr (has_static_resolve_request_v<T,
                                                   RemoveRvalueReferences>) {
            return resolve_static<T, RemoveRvalueReferences>(context);
        } else if constexpr (has_parent_v) {
            if constexpr (static_resolve_status_v<
                              T, RemoveRvalueReferences> ==
                          binding_selection_status::kNotFound) {
                if (parent_) {
                    return resolve_parent<T, RemoveRvalueReferences>();
                }
            }
            return resolve_static<T, RemoveRvalueReferences>(context);
        } else {
            return resolve_static<T, RemoveRvalueReferences>(context);
        }
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache = true,
              typename Key = void,
              typename R = resolve_result_t<T, RemoveRvalueReferences>>
    R resolve(static_context_type& context) {
        if constexpr (has_static_resolve_request_v<T, RemoveRvalueReferences,
                                                   Key>) {
            return resolve_static<T, RemoveRvalueReferences, Key>(context);
        } else if constexpr (has_parent_v) {
            if constexpr (static_resolve_status_v<
                              T, RemoveRvalueReferences, Key> ==
                          binding_selection_status::kNotFound) {
                if (parent_) {
                    return resolve_parent<T, RemoveRvalueReferences, Key>();
                }
            }
            return resolve_static<T, RemoveRvalueReferences, Key>(context);
        } else {
            return resolve_static<T, RemoveRvalueReferences, Key>(context);
        }
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache,
              typename Key,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve(static_context_type& context, key<Key>) {
        if constexpr (has_static_resolve_request_v<T, RemoveRvalueReferences,
                                                   Key>) {
            return resolve_static<T, RemoveRvalueReferences, Key>(context);
        } else if constexpr (has_parent_v) {
            if constexpr (static_resolve_status_v<
                              T, RemoveRvalueReferences, Key> ==
                          binding_selection_status::kNotFound) {
                if (parent_) {
                    return resolve_parent<T, RemoveRvalueReferences, Key>();
                }
            }
            return resolve_static<T, RemoveRvalueReferences, Key>(context);
        } else {
            return resolve_static<T, RemoveRvalueReferences, Key>(context);
        }
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache = true,
              typename R = resolve_result_t<T, RemoveRvalueReferences>>
    R resolve(runtime_context& context, none_t) {
        return resolve<T, RemoveRvalueReferences, CheckCache>(context);
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache = true,
              typename Key = void,
              typename R = resolve_result_t<T, RemoveRvalueReferences>>
    R resolve(runtime_context& context) {
        if constexpr (collection_traits<R>::is_collection) {
            if constexpr (has_parent_v) {
                if (parent_ && count_collection<R, Key>() == 0) {
                    return resolve_parent<T, RemoveRvalueReferences,
                                          CheckCache, Key>(context);
                }
            }
            return construct_collection<R>(context,
                                           binding_collection_append{},
                                           collection_key<Key>());
        } else {
            if constexpr (has_parent_v) {
                if constexpr (static_resolve_status_v<
                                  T, RemoveRvalueReferences, Key> ==
                              binding_selection_status::kNotFound) {
                    if (parent_ &&
                        resolve_binding_status<T, RemoveRvalueReferences,
                                               Key>() ==
                            binding_selection_status::kNotFound) {
                        return resolve_parent<T, RemoveRvalueReferences,
                                              CheckCache, Key>(context);
                    }
                }
            }
            using request_type = R;
            runtime_binding_candidate<T, RemoveRvalueReferences, Key,
                                      request_type>
                runtime{*this};
            static_binding_candidate<request_type, Key> static_binding{*this};
            auto sources = make_two_binding_sources(
                runtime, static_binding, runtime,
                binding_resolution_policy::kAmbiguousOnConflict);
            return resolve_from_binding_sources<T, request_type>(
                context, sources);
        }
    }

    template <typename T, bool RemoveRvalueReferences, bool CheckCache,
              typename Key,
              typename R = resolve_request_t<T, RemoveRvalueReferences>>
    R resolve(runtime_context& context, key<Key>) {
        return resolve<T, RemoveRvalueReferences, CheckCache, Key>(context);
    }

  private:
    friend class runtime_registration_api<self_type>;

    runtime_registry_type& runtime_registry_ref() { return runtime_registry_; }

    self_type& runtime_registration_parent() { return *this; }

    runtime_registry_type runtime_registry_;
    static_registry_type static_registry_;
    static_state static_state_;
    parent_container_type* parent_ = nullptr;
};



template <typename... Params> struct container_base;
template <typename Param, typename Enable = void>
struct container_base_from_parameter;
template <typename Param, typename Parent, typename Enable = void>
struct container_base_from_static_parent;
struct container_base_runtime_two_parameter_tag {};
struct container_base_static_parent_tag {};
struct container_base_invalid_two_parameter_tag {};

template <typename First>
using container_base_two_parameter_tag_t = std::conditional_t<
    is_runtime_container_traits_v<First>, container_base_runtime_two_parameter_tag,
    std::conditional_t<is_static_registry_v<First> ||
                           is_bindings_wrapper_v<First>,
                       container_base_static_parent_tag,
                       container_base_invalid_two_parameter_tag>>;

template <> struct container_base<> {
    using type = runtime_container<dynamic_container_traits>;
};

template <typename Param> struct container_base<Param> {
    using type = typename container_base_from_parameter<Param>::type;
};

template <typename First, typename Second>
struct container_base<First, Second> {
    using type = typename container_base<
        container_base_two_parameter_tag_t<First>, First, Second>::type;
};

template <typename First, typename Second>
struct container_base<container_base_runtime_two_parameter_tag, First,
                      Second> {
    using type = runtime_container<First, Second, void>;
};

template <typename First, typename Second>
struct container_base<container_base_static_parent_tag, First, Second> {
    using type = typename container_base_from_static_parent<First, Second>::type;
};

template <typename First, typename Second>
struct container_base<container_base_invalid_two_parameter_tag, First,
                      Second> {
    static_assert(
        container_dependent_false_v<First, Second>,
        "container<T, U> requires runtime traits + allocator or "
        "compile-time bindings + parent container");
};

template <typename First, typename Second, typename Third>
struct container_base<First, Second, Third> {
    static_assert(
        is_runtime_container_traits_v<First>,
        "container<T, U, V> requires runtime traits + allocator + parent");
    using type = runtime_container<First, Second, Third>;
};

template <typename First, typename Second, typename Third, typename... Rest>
struct container_base<First, Second, Third, Rest...> {
    static_assert(container_dependent_false_v<First, Second, Third, Rest...>,
                  "container<...> expects runtime traits or a single static "
                  "bindings source");
};

template <typename... Params>
using container_base_t = typename container_base<Params...>::type;

template <typename Param, typename Enable>
struct container_base_from_parameter {
    static_assert(container_dependent_false_v<Param>,
                  "container<T> requires runtime container traits or "
                  "compile-time bindings<...>");
};

template <typename Param>
struct container_base_from_parameter<
    Param, std::enable_if_t<is_runtime_container_traits_v<Param>>> {
    using type = runtime_container<Param, typename Param::allocator_type, void>;
};

template <typename Param>
struct container_base_from_parameter<
    Param, std::enable_if_t<is_static_registry_v<Param>>> {
    using type = container_with_static_bindings<Param>;
};

template <typename Param>
struct container_base_from_parameter<
    Param, std::enable_if_t<is_bindings_wrapper_v<Param>>> {
    using static_registry_type = bindings_wrapper_registry_t<Param>;

    static_assert(is_static_registry_v<static_registry_type>,
                  "container<bindings<...>> requires a valid compile-time "
                  "bindings source");

    using type = container_with_static_bindings<static_registry_type>;
};

template <typename Param, typename Parent, typename Enable>
struct container_base_from_static_parent {
    static_assert(container_dependent_false_v<Param, Parent>,
                  "container<bindings<...>, Parent> requires a valid "
                  "compile-time bindings source");
};

template <typename Param, typename Parent>
struct container_base_from_static_parent<
    Param, Parent, std::enable_if_t<is_static_registry_v<Param>>> {
    using type = container_with_static_bindings<Param, Parent>;
};

template <typename Param, typename Parent>
struct container_base_from_static_parent<
    Param, Parent, std::enable_if_t<is_bindings_wrapper_v<Param>>> {
    using static_registry_type = bindings_wrapper_registry_t<Param>;

    static_assert(is_static_registry_v<static_registry_type>,
                  "container<bindings<...>, Parent> requires a valid "
                  "compile-time bindings source");

    using type = container_with_static_bindings<static_registry_type, Parent>;
};



template <typename... Params>
class container : public container_base_t<Params...> {
    using container_base_type = container_base_t<Params...>;

  public:
    using container_base_type::container_base_type;
};
} // export namespace silicon::di

#ifdef _MSC_VER
#pragma warning(pop)
#endif



// ==============================================================================
// ==  index  —  index collections (array / map / unordered_map)
// ==============================================================================

// --- index/array.h ---




export namespace silicon::di {
namespace index_type {
template <size_t N> struct array {};
} // namespace index_type

template <typename Key, typename Value, typename Allocator, size_t N>
struct index_collection<Key, Value, Allocator, index_type::array<N>> {
    static_assert(std::is_integral_v<Key> && std::is_unsigned_v<Key>);

    index_collection(Allocator&) {}

    as_expected_t<bool> emplace(Key key, Value value) {
        if (key >= array_.size())
            return std::unexpected(make_type_index_out_of_range_exception(
                key, array_.size()));
        if (!array_[key]) {
            array_[key] = value;
            return true;
        }
        return false;
    }

    Value* find(Key key) {
        if (key < array_.size() && array_[key])
            return &array_[key];
        return nullptr;
    }

  private:
    std::array<Value, N> array_{};
};

} // export namespace silicon::di

// --- index/map.h ---




export namespace silicon::di {
namespace index_type {
struct map {};
} // namespace index_type

template <typename Key, typename Value, typename Allocator>
struct index_collection<Key, Value, Allocator, index_type::map> {
    static_assert(!is_static_allocator_v<Allocator>);

    index_collection(Allocator& allocator) : map_(allocator) {}

    bool emplace(Key&& key, Value value) {
        auto pb = map_.emplace(std::move(key), value);
        return pb.second;
    }

    Value* find(const Key& key) {
        auto it = map_.find(key);
        return it != map_.end() ? &it->second : nullptr;
    }

  private:
    using allocator_type = typename std::allocator_traits<
        Allocator>::template rebind_alloc<std::pair<const Key, Value>>;

    std::map<Key, Value, std::less<Key>, allocator_type> map_;
};
} // export namespace silicon::di

// --- index/unordered_map.h ---




export namespace silicon::di {
namespace index_type {
struct unordered_map {};
} // namespace index_type

template <typename Key, typename Value, typename Allocator>
struct index_collection<Key, Value, Allocator, index_type::unordered_map> {
    static_assert(!is_static_allocator_v<Allocator>);

    index_collection(Allocator& allocator) : map_(allocator) {}

    bool emplace(Key&& key, Value value) {
        auto pb = map_.emplace(std::move(key), value);
        return pb.second;
    }

    Value* find(const Key& key) {
        auto it = map_.find(key);
        return it != map_.end() ? &it->second : nullptr;
    }

  private:
    using allocator_type = typename std::allocator_traits<
        Allocator>::template rebind_alloc<std::pair<const Key, Value>>;

    std::unordered_map<Key, Value, std::hash<Key>, std::equal_to<Key>,
                       allocator_type>
        map_;
};
} // export namespace silicon::di


// ==============================================================================
// ==  registration  —  type registration, annotations & requirements
// ==============================================================================

// --- registration/constructor.h ---

// Intentionally without any includes

export namespace silicon::di {

template <typename...> struct constructor;
#define SILICON_DI_CONSTRUCTOR(...)                                                 \
    using di_constructor_type [[maybe_unused]] =                            \
        ::silicon::di::constructor<__VA_ARGS__>;                                     \
    __VA_ARGS__

} // export namespace silicon::di


// ==============================================================================
// ==  type  —  type traits, descriptors, type lists & maps
// ==============================================================================

// --- type/type_name.h ---



export namespace silicon::di {

template <typename T> std::string type_name() {
    std::string name;
    append_type_name(name, describe_type<T>());
    return name;
}

} // export namespace silicon::di
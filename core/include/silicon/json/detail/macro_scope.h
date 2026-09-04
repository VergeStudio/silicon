







#pragma once







#include <silicon/json/thirdparty/hedley/hedley.h>
#include <utility>




#include <silicon/json/detail/abi_macros.h>


#if !defined(JSON_SKIP_UNSUPPORTED_COMPILER_CHECK)
#    if defined(__clang__)
#        if (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__) < 30400
#            error "unsupported Clang version - see https://github.com/silicon/json#supported-compilers"
#        endif
#    elif defined(__GNUC__) && !(defined(__ICC) || defined(__INTEL_COMPILER))
#        if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) < 40800
#            error "unsupported GCC version - see https://github.com/silicon/json#supported-compilers"
#        endif
#    endif
#endif



#if !defined(JSON_HAS_CPP_20) && !defined(JSON_HAS_CPP_17) && !defined(JSON_HAS_CPP_14) && !defined(JSON_HAS_CPP_11)
#    if (defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#        define JSON_HAS_CPP_20
#        define JSON_HAS_CPP_17
#        define JSON_HAS_CPP_14
#    elif (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_HAS_CXX17) && _HAS_CXX17 == 1)
#        define JSON_HAS_CPP_17
#        define JSON_HAS_CPP_14
#    elif (defined(__cplusplus) && __cplusplus >= 201402L) || (defined(_HAS_CXX14) && _HAS_CXX14 == 1)
#        define JSON_HAS_CPP_14
#    endif

#    define JSON_HAS_CPP_11
#endif

#ifdef __has_include
#    if __has_include(<version>)
#        include <version>
#    endif
#endif

#if !defined(JSON_HAS_FILESYSTEM) && !defined(JSON_HAS_EXPERIMENTAL_FILESYSTEM)
#    ifdef JSON_HAS_CPP_17
#        if defined(__cpp_lib_filesystem)
#            define JSON_HAS_FILESYSTEM 1
#        elif defined(__cpp_lib_experimental_filesystem)
#            define JSON_HAS_EXPERIMENTAL_FILESYSTEM 1
#        elif !defined(__has_include)
#            define JSON_HAS_EXPERIMENTAL_FILESYSTEM 1
#        elif __has_include(<filesystem>)
#            define JSON_HAS_FILESYSTEM 1
#        elif __has_include(<experimental/filesystem>)
#            define JSON_HAS_EXPERIMENTAL_FILESYSTEM 1
#        endif


#        if defined(__MINGW32__) && defined(__GNUC__) && __GNUC__ == 8
#            undef JSON_HAS_FILESYSTEM
#            undef JSON_HAS_EXPERIMENTAL_FILESYSTEM
#        endif


#        if defined(__GNUC__) && !defined(__clang__) && __GNUC__ < 8
#            undef JSON_HAS_FILESYSTEM
#            undef JSON_HAS_EXPERIMENTAL_FILESYSTEM
#        endif


#        if defined(__clang_major__) && __clang_major__ < 7
#            undef JSON_HAS_FILESYSTEM
#            undef JSON_HAS_EXPERIMENTAL_FILESYSTEM
#        endif


#        if defined(_MSC_VER) && _MSC_VER < 1914
#            undef JSON_HAS_FILESYSTEM
#            undef JSON_HAS_EXPERIMENTAL_FILESYSTEM
#        endif


#        if defined(__IPHONE_OS_VERSION_MIN_REQUIRED) && __IPHONE_OS_VERSION_MIN_REQUIRED < 130000
#            undef JSON_HAS_FILESYSTEM
#            undef JSON_HAS_EXPERIMENTAL_FILESYSTEM
#        endif


#        if defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101500
#            undef JSON_HAS_FILESYSTEM
#            undef JSON_HAS_EXPERIMENTAL_FILESYSTEM
#        endif
#    endif
#endif

#ifndef JSON_HAS_EXPERIMENTAL_FILESYSTEM
#    define JSON_HAS_EXPERIMENTAL_FILESYSTEM 0
#endif

#ifndef JSON_HAS_FILESYSTEM
#    define JSON_HAS_FILESYSTEM 0
#endif

#ifndef JSON_HAS_THREE_WAY_COMPARISON
#    if defined(__cpp_impl_three_way_comparison) && __cpp_impl_three_way_comparison >= 201907L && defined(__cpp_lib_three_way_comparison) && __cpp_lib_three_way_comparison >= 201907L
#        define JSON_HAS_THREE_WAY_COMPARISON 1
#    else
#        define JSON_HAS_THREE_WAY_COMPARISON 0
#    endif
#endif

#ifndef JSON_HAS_RANGES

#    if defined(__GLIBCXX__) && __GLIBCXX__ == 20210427
#        define JSON_HAS_RANGES 0
#    elif defined(__cpp_lib_ranges)
#        define JSON_HAS_RANGES 1
#    else
#        define JSON_HAS_RANGES 0
#    endif
#endif

#ifndef JSON_HAS_STATIC_RTTI
#    if !defined(_HAS_STATIC_RTTI) || _HAS_STATIC_RTTI != 0
#        define JSON_HAS_STATIC_RTTI 1
#    else
#        define JSON_HAS_STATIC_RTTI 0
#    endif
#endif

#ifdef JSON_HAS_CPP_17
#    define JSON_INLINE_VARIABLE inline
#else
#    define JSON_INLINE_VARIABLE
#endif

#if JSON_HEDLEY_HAS_ATTRIBUTE(no_unique_address)
#    define JSON_NO_UNIQUE_ADDRESS [[no_unique_address]]
#else
#    define JSON_NO_UNIQUE_ADDRESS
#endif


#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdocumentation"
#    pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#endif


#if (defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)) && !defined(JSON_NOEXCEPTION)
#    define JSON_THROW(exception) throw exception
#    define JSON_TRY try
#    define JSON_CATCH(exception) catch(exception)
#    define JSON_INTERNAL_CATCH(exception) catch(exception)
#else
#    include <cstdlib>
#    define JSON_THROW(exception) std::abort()
#    define JSON_TRY if(true)
#    define JSON_CATCH(exception) if(false)
#    define JSON_INTERNAL_CATCH(exception) if(false)
#endif


#if defined(JSON_THROW_USER)
#    undef JSON_THROW
#    define JSON_THROW JSON_THROW_USER
#endif
#if defined(JSON_TRY_USER)
#    undef JSON_TRY
#    define JSON_TRY JSON_TRY_USER
#endif
#if defined(JSON_CATCH_USER)
#    undef JSON_CATCH
#    define JSON_CATCH JSON_CATCH_USER
#    undef JSON_INTERNAL_CATCH
#    define JSON_INTERNAL_CATCH JSON_CATCH_USER
#endif
#if defined(JSON_INTERNAL_CATCH_USER)
#    undef JSON_INTERNAL_CATCH
#    define JSON_INTERNAL_CATCH JSON_INTERNAL_CATCH_USER
#endif


#if !defined(JSON_ASSERT)
#    include <cassert>
#    define JSON_ASSERT(x) assert(x)
#endif


#if defined(JSON_TESTS_PRIVATE)
#    define JSON_PRIVATE_UNLESS_TESTED public
#else
#    define JSON_PRIVATE_UNLESS_TESTED private
#endif


#define SILICON_JSON_SERIALIZE_ENUM(ENUM_TYPE, ...)                                                                           \
    template<typename BasicJsonType>                                                                                          \
    inline void to_json(BasicJsonType &j, const ENUM_TYPE &e) {                                                               \
        static_assert(std::is_enum<ENUM_TYPE>::value, #ENUM_TYPE " must be an enum!");                                        \
        static const std::pair<ENUM_TYPE, BasicJsonType> m[] = __VA_ARGS__;                                                   \
        auto it = std::find_if(std::begin(m), std::end(m), [e](const std::pair<ENUM_TYPE, BasicJsonType> &ej_pair) -> bool {  \
            return ej_pair.first == e;                                                                                        \
        });                                                                                                                   \
        j = ((it != std::end(m)) ? it : std::begin(m))->second;                                                               \
    }                                                                                                                         \
    template<typename BasicJsonType>                                                                                          \
    inline void from_json(const BasicJsonType &j, ENUM_TYPE &e) {                                                             \
        static_assert(std::is_enum<ENUM_TYPE>::value, #ENUM_TYPE " must be an enum!");                                        \
        static const std::pair<ENUM_TYPE, BasicJsonType> m[] = __VA_ARGS__;                                                   \
        auto it = std::find_if(std::begin(m), std::end(m), [&j](const std::pair<ENUM_TYPE, BasicJsonType> &ej_pair) -> bool { \
            return ej_pair.second == j;                                                                                       \
        });                                                                                                                   \
        e = ((it != std::end(m)) ? it : std::begin(m))->first;                                                                \
    }




#define silicon_BASIC_JSON_TPL_DECLARATION \
    template<template<typename, typename, typename...> class ObjectType, template<typename, typename...> class ArrayType, class StringType, class BooleanType, class NumberIntegerType, class NumberUnsignedType, class NumberFloatType, template<typename> class AllocatorType, template<typename, typename = void> class JSONSerializer, class BinaryType, class CustomBaseClass>

#define silicon_BASIC_JSON_TPL \
    basic_json<ObjectType, ArrayType, StringType, BooleanType, NumberIntegerType, NumberUnsignedType, NumberFloatType, AllocatorType, JSONSerializer, BinaryType, CustomBaseClass>



#define SILICON_JSON_EXPAND(x) x
#define SILICON_JSON_GET_MACRO(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34, _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, NAME, ...) NAME
#define SILICON_JSON_PASTE(...) SILICON_JSON_EXPAND(SILICON_JSON_GET_MACRO(__VA_ARGS__, SILICON_JSON_PASTE64, SILICON_JSON_PASTE63, SILICON_JSON_PASTE62, SILICON_JSON_PASTE61, SILICON_JSON_PASTE60, SILICON_JSON_PASTE59, SILICON_JSON_PASTE58, SILICON_JSON_PASTE57, SILICON_JSON_PASTE56, SILICON_JSON_PASTE55, SILICON_JSON_PASTE54, SILICON_JSON_PASTE53, SILICON_JSON_PASTE52, SILICON_JSON_PASTE51, SILICON_JSON_PASTE50, SILICON_JSON_PASTE49, SILICON_JSON_PASTE48, SILICON_JSON_PASTE47, SILICON_JSON_PASTE46, SILICON_JSON_PASTE45, SILICON_JSON_PASTE44, SILICON_JSON_PASTE43, SILICON_JSON_PASTE42, SILICON_JSON_PASTE41, SILICON_JSON_PASTE40, SILICON_JSON_PASTE39, SILICON_JSON_PASTE38, SILICON_JSON_PASTE37, SILICON_JSON_PASTE36, SILICON_JSON_PASTE35, SILICON_JSON_PASTE34, SILICON_JSON_PASTE33, SILICON_JSON_PASTE32, SILICON_JSON_PASTE31, SILICON_JSON_PASTE30, SILICON_JSON_PASTE29, SILICON_JSON_PASTE28, SILICON_JSON_PASTE27, SILICON_JSON_PASTE26, SILICON_JSON_PASTE25, SILICON_JSON_PASTE24, SILICON_JSON_PASTE23, SILICON_JSON_PASTE22, SILICON_JSON_PASTE21, SILICON_JSON_PASTE20, SILICON_JSON_PASTE19, SILICON_JSON_PASTE18, SILICON_JSON_PASTE17, SILICON_JSON_PASTE16, SILICON_JSON_PASTE15, SILICON_JSON_PASTE14, SILICON_JSON_PASTE13, SILICON_JSON_PASTE12, SILICON_JSON_PASTE11, SILICON_JSON_PASTE10, SILICON_JSON_PASTE9, SILICON_JSON_PASTE8, SILICON_JSON_PASTE7, SILICON_JSON_PASTE6, SILICON_JSON_PASTE5, SILICON_JSON_PASTE4, SILICON_JSON_PASTE3, SILICON_JSON_PASTE2, SILICON_JSON_PASTE1)(__VA_ARGS__))
#define SILICON_JSON_PASTE2(func, v1) func(v1)
#define SILICON_JSON_PASTE3(func, v1, v2) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE2(func, v2)
#define SILICON_JSON_PASTE4(func, v1, v2, v3) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE3(func, v2, v3)
#define SILICON_JSON_PASTE5(func, v1, v2, v3, v4) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE4(func, v2, v3, v4)
#define SILICON_JSON_PASTE6(func, v1, v2, v3, v4, v5) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE5(func, v2, v3, v4, v5)
#define SILICON_JSON_PASTE7(func, v1, v2, v3, v4, v5, v6) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE6(func, v2, v3, v4, v5, v6)
#define SILICON_JSON_PASTE8(func, v1, v2, v3, v4, v5, v6, v7) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE7(func, v2, v3, v4, v5, v6, v7)
#define SILICON_JSON_PASTE9(func, v1, v2, v3, v4, v5, v6, v7, v8) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE8(func, v2, v3, v4, v5, v6, v7, v8)
#define SILICON_JSON_PASTE10(func, v1, v2, v3, v4, v5, v6, v7, v8, v9) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE9(func, v2, v3, v4, v5, v6, v7, v8, v9)
#define SILICON_JSON_PASTE11(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE10(func, v2, v3, v4, v5, v6, v7, v8, v9, v10)
#define SILICON_JSON_PASTE12(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE11(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)
#define SILICON_JSON_PASTE13(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE12(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12)
#define SILICON_JSON_PASTE14(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE13(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13)
#define SILICON_JSON_PASTE15(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE14(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14)
#define SILICON_JSON_PASTE16(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE15(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15)
#define SILICON_JSON_PASTE17(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE16(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16)
#define SILICON_JSON_PASTE18(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE17(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17)
#define SILICON_JSON_PASTE19(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE18(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18)
#define SILICON_JSON_PASTE20(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE19(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19)
#define SILICON_JSON_PASTE21(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE20(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20)
#define SILICON_JSON_PASTE22(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE21(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21)
#define SILICON_JSON_PASTE23(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE22(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22)
#define SILICON_JSON_PASTE24(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE23(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23)
#define SILICON_JSON_PASTE25(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE24(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24)
#define SILICON_JSON_PASTE26(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE25(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25)
#define SILICON_JSON_PASTE27(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE26(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26)
#define SILICON_JSON_PASTE28(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE27(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27)
#define SILICON_JSON_PASTE29(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE28(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28)
#define SILICON_JSON_PASTE30(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE29(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29)
#define SILICON_JSON_PASTE31(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE30(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30)
#define SILICON_JSON_PASTE32(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE31(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31)
#define SILICON_JSON_PASTE33(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE32(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32)
#define SILICON_JSON_PASTE34(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE33(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33)
#define SILICON_JSON_PASTE35(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE34(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34)
#define SILICON_JSON_PASTE36(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE35(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35)
#define SILICON_JSON_PASTE37(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE36(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36)
#define SILICON_JSON_PASTE38(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE37(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37)
#define SILICON_JSON_PASTE39(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE38(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38)
#define SILICON_JSON_PASTE40(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE39(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39)
#define SILICON_JSON_PASTE41(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE40(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40)
#define SILICON_JSON_PASTE42(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE41(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41)
#define SILICON_JSON_PASTE43(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE42(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42)
#define SILICON_JSON_PASTE44(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE43(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43)
#define SILICON_JSON_PASTE45(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE44(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44)
#define SILICON_JSON_PASTE46(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE45(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45)
#define SILICON_JSON_PASTE47(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE46(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46)
#define SILICON_JSON_PASTE48(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE47(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47)
#define SILICON_JSON_PASTE49(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE48(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48)
#define SILICON_JSON_PASTE50(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE49(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49)
#define SILICON_JSON_PASTE51(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE50(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50)
#define SILICON_JSON_PASTE52(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE51(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51)
#define SILICON_JSON_PASTE53(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE52(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52)
#define SILICON_JSON_PASTE54(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE53(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53)
#define SILICON_JSON_PASTE55(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE54(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54)
#define SILICON_JSON_PASTE56(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE55(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55)
#define SILICON_JSON_PASTE57(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE56(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56)
#define SILICON_JSON_PASTE58(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE57(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57)
#define SILICON_JSON_PASTE59(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE58(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58)
#define SILICON_JSON_PASTE60(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE59(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59)
#define SILICON_JSON_PASTE61(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE60(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60)
#define SILICON_JSON_PASTE62(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE61(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61)
#define SILICON_JSON_PASTE63(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE62(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62)
#define SILICON_JSON_PASTE64(func, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62, v63) SILICON_JSON_PASTE2(func, v1) SILICON_JSON_PASTE63(func, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17, v18, v19, v20, v21, v22, v23, v24, v25, v26, v27, v28, v29, v30, v31, v32, v33, v34, v35, v36, v37, v38, v39, v40, v41, v42, v43, v44, v45, v46, v47, v48, v49, v50, v51, v52, v53, v54, v55, v56, v57, v58, v59, v60, v61, v62, v63)

#define SILICON_JSON_TO(v1) silicon_json_j[#v1] = silicon_json_t.v1;
#define SILICON_JSON_FROM(v1) silicon_json_j.at(#v1).get_to(silicon_json_t.v1);
#define SILICON_JSON_FROM_WITH_DEFAULT(v1) silicon_json_t.v1 = silicon_json_j.value(#v1, silicon_json_default_obj.v1);


#define silicon_DEFINE_TYPE_INTRUSIVE(Type, ...)                                                                                                                        \
    friend void to_json(silicon::json::impl::json &silicon_json_j, const Type &silicon_json_t) { SILICON_JSON_EXPAND(SILICON_JSON_PASTE(SILICON_JSON_TO, __VA_ARGS__)) } \
    friend void from_json(const silicon::json::impl::json &silicon_json_j, Type &silicon_json_t) { SILICON_JSON_EXPAND(SILICON_JSON_PASTE(SILICON_JSON_FROM, __VA_ARGS__)) }

#define silicon_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Type, ...)                                                                                                           \
    friend void to_json(silicon::json::impl::json &silicon_json_j, const Type &silicon_json_t) { SILICON_JSON_EXPAND(SILICON_JSON_PASTE(SILICON_JSON_TO, __VA_ARGS__)) } \
    friend void from_json(const silicon::json::impl::json &silicon_json_j, Type &silicon_json_t) {                                                                       \
        const Type silicon_json_default_obj{};                                                                                                                          \
        SILICON_JSON_EXPAND(SILICON_JSON_PASTE(SILICON_JSON_FROM_WITH_DEFAULT, __VA_ARGS__))                                                                            \
    }

#define silicon_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(Type, ...) \
    friend void to_json(silicon::json::impl::json &silicon_json_j, const Type &silicon_json_t) { SILICON_JSON_EXPAND(SILICON_JSON_PASTE(SILICON_JSON_TO, __VA_ARGS__)) }


#define silicon_DEFINE_TYPE_NON_INTRUSIVE(Type, ...)                                                                                                                    \
    inline void to_json(silicon::json::impl::json &silicon_json_j, const Type &silicon_json_t) { SILICON_JSON_EXPAND(SILICON_JSON_PASTE(SILICON_JSON_TO, __VA_ARGS__)) } \
    inline void from_json(const silicon::json::impl::json &silicon_json_j, Type &silicon_json_t) { SILICON_JSON_EXPAND(SILICON_JSON_PASTE(SILICON_JSON_FROM, __VA_ARGS__)) }

#define silicon_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(Type, ...) \
    inline void to_json(silicon::json::impl::json &silicon_json_j, const Type &silicon_json_t) { SILICON_JSON_EXPAND(SILICON_JSON_PASTE(SILICON_JSON_TO, __VA_ARGS__)) }

#define silicon_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Type, ...)                                                                                                       \
    inline void to_json(silicon::json::impl::json &silicon_json_j, const Type &silicon_json_t) { SILICON_JSON_EXPAND(SILICON_JSON_PASTE(SILICON_JSON_TO, __VA_ARGS__)) } \
    inline void from_json(const silicon::json::impl::json &silicon_json_j, Type &silicon_json_t) {                                                                       \
        const Type silicon_json_default_obj{};                                                                                                                          \
        SILICON_JSON_EXPAND(SILICON_JSON_PASTE(SILICON_JSON_FROM_WITH_DEFAULT, __VA_ARGS__))                                                                            \
    }







#define silicon_CAN_CALL_STD_FUNC_IMPL(std_name)                                      \
    namespace detail {                                                                \
    using std::std_name;                                                              \
                                                                                      \
    template<typename... T>                                                           \
    using result_of_##std_name = decltype(std_name(std::declval<T>()...));            \
    }                                                                                 \
                                                                                      \
    namespace detail2 {                                                               \
    struct std_name##_tag {                                                           \
    };                                                                                \
                                                                                      \
    template<typename... T>                                                           \
    std_name##_tag std_name(T &&...);                                                 \
                                                                                      \
    template<typename... T>                                                           \
    using result_of_##std_name = decltype(std_name(std::declval<T>()...));            \
                                                                                      \
    template<typename... T>                                                           \
    struct would_call_std_##std_name {                                                \
        static constexpr auto const value = ::silicon::json::impl::detail::            \
                is_detected_exact<std_name##_tag, result_of_##std_name, T...>::value; \
    };                                                                                \
    }                                                          \
                                                                                      \
    template<typename... T>                                                           \
    struct would_call_std_##std_name: detail2::would_call_std_##std_name<T...> {      \
    }

#ifndef JSON_USE_IMPLICIT_CONVERSIONS
#    define JSON_USE_IMPLICIT_CONVERSIONS 1
#endif

#if JSON_USE_IMPLICIT_CONVERSIONS
#    define JSON_EXPLICIT
#else
#    define JSON_EXPLICIT explicit
#endif

#ifndef JSON_DISABLE_ENUM_SERIALIZATION
#    define JSON_DISABLE_ENUM_SERIALIZATION 0
#endif

#ifndef JSON_USE_GLOBAL_UDLS
#    define JSON_USE_GLOBAL_UDLS 1
#endif

module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <cstring>
#include <string>
#include <utility>

export module silicon.json:detail.string_concat;

import :detail.meta.cpp_future;
import :detail.meta.detected;

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

export inline std::size_t concat_length() {
    return 0;
}

export template<typename... Args>
inline std::size_t concat_length(const char *cstr, const Args &...rest);

export template<typename StringType, typename... Args>
inline std::size_t concat_length(const StringType &str, const Args &...rest);

export template<typename... Args>
inline std::size_t concat_length(const char , const Args &...rest) {
    return 1 + concat_length(rest...);
}

export template<typename... Args>
inline std::size_t concat_length(const char *cstr, const Args &...rest) {

    return ::strlen(cstr) + concat_length(rest...);
}

export template<typename StringType, typename... Args>
inline std::size_t concat_length(const StringType &str, const Args &...rest) {
    return str.size() + concat_length(rest...);
}

export template<typename OutStringType>
inline void concat_into(OutStringType & ) {}

template<typename StringType, typename Arg>
using string_can_append = decltype(std::declval<StringType &>().append(std::declval<Arg &&>()));

export template<typename StringType, typename Arg>
using detect_string_can_append = is_detected<string_can_append, StringType, Arg>;

export template<typename StringType, typename Arg>
using string_can_append_op = decltype(std::declval<StringType &>() += std::declval<Arg &&>());

export template<typename StringType, typename Arg>
using detect_string_can_append_op = is_detected<string_can_append_op, StringType, Arg>;

export template<typename StringType, typename Arg>
using string_can_append_iter = decltype(std::declval<StringType &>().append(std::declval<const Arg &>().begin(), std::declval<const Arg &>().end()));

export template<typename StringType, typename Arg>
using detect_string_can_append_iter = is_detected<string_can_append_iter, StringType, Arg>;

export template<typename StringType, typename Arg>
using string_can_append_data = decltype(std::declval<StringType &>().append(std::declval<const Arg &>().data(), std::declval<const Arg &>().size()));

export template<typename StringType, typename Arg>
using detect_string_can_append_data = is_detected<string_can_append_data, StringType, Arg>;

export template<typename OutStringType, typename Arg, typename... Args, enable_if_t<!detect_string_can_append<OutStringType, Arg>::value && detect_string_can_append_op<OutStringType, Arg>::value, int> = 0>
inline void concat_into(OutStringType &out, Arg &&arg, Args &&...rest);

export template<typename OutStringType, typename Arg, typename... Args, enable_if_t<!detect_string_can_append<OutStringType, Arg>::value && !detect_string_can_append_op<OutStringType, Arg>::value && detect_string_can_append_iter<OutStringType, Arg>::value, int> = 0>
inline void concat_into(OutStringType &out, const Arg &arg, Args &&...rest);

export template<typename OutStringType, typename Arg, typename... Args, enable_if_t<!detect_string_can_append<OutStringType, Arg>::value && !detect_string_can_append_op<OutStringType, Arg>::value && !detect_string_can_append_iter<OutStringType, Arg>::value && detect_string_can_append_data<OutStringType, Arg>::value, int> = 0>
inline void concat_into(OutStringType &out, const Arg &arg, Args &&...rest);

export template<typename OutStringType, typename Arg, typename... Args, enable_if_t<detect_string_can_append<OutStringType, Arg>::value, int> = 0>
inline void concat_into(OutStringType &out, Arg &&arg, Args &&...rest) {
    out.append(std::forward<Arg>(arg));
    concat_into(out, std::forward<Args>(rest)...);
}

export template<typename OutStringType, typename Arg, typename... Args, enable_if_t<!detect_string_can_append<OutStringType, Arg>::value && detect_string_can_append_op<OutStringType, Arg>::value, int> >
inline void concat_into(OutStringType &out, Arg &&arg, Args &&...rest) {
    out += std::forward<Arg>(arg);
    concat_into(out, std::forward<Args>(rest)...);
}

export template<typename OutStringType, typename Arg, typename... Args, enable_if_t<!detect_string_can_append<OutStringType, Arg>::value && !detect_string_can_append_op<OutStringType, Arg>::value && detect_string_can_append_iter<OutStringType, Arg>::value, int> >
inline void concat_into(OutStringType &out, const Arg &arg, Args &&...rest) {
    out.append(arg.begin(), arg.end());
    concat_into(out, std::forward<Args>(rest)...);
}

export template<typename OutStringType, typename Arg, typename... Args, enable_if_t<!detect_string_can_append<OutStringType, Arg>::value && !detect_string_can_append_op<OutStringType, Arg>::value && !detect_string_can_append_iter<OutStringType, Arg>::value && detect_string_can_append_data<OutStringType, Arg>::value, int> >
inline void concat_into(OutStringType &out, const Arg &arg, Args &&...rest) {
    out.append(arg.data(), arg.size());
    concat_into(out, std::forward<Args>(rest)...);
}

export template<typename OutStringType = std::string, typename... Args>
inline OutStringType concat(Args &&...args) {
    OutStringType str;
    str.reserve(concat_length(args...));
    concat_into(str, std::forward<Args>(args)...);
    return str;
}

}
SILICON_JSON_NAMESPACE_END

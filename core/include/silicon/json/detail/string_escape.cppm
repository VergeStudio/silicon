












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>

export module silicon.json:detail.string_escape;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {


export template<typename StringType>
inline void replace_substring(StringType &s, const StringType &f, const StringType &t) {
    JSON_ASSERT(!f.empty());
    for(auto pos = s.find(f);
        pos != StringType::npos;
        s.replace(pos, f.size(), t),
        pos = s.find(f, pos + t.size()))
    {}
}


export template<typename StringType>
inline StringType escape(StringType s) {
    replace_substring(s, StringType{"~"}, StringType{"~0"});
    replace_substring(s, StringType{"/"}, StringType{"~1"});
    return s;
}


export template<typename StringType>
inline void unescape(StringType &s) {
    replace_substring(s, StringType{"~1"}, StringType{"/"});
    replace_substring(s, StringType{"~0"}, StringType{"~"});
}

}
SILICON_JSON_NAMESPACE_END

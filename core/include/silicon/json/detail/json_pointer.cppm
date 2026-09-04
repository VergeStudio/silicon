












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#ifndef JSON_NO_IO
#    include <iosfwd>
#endif
#include <limits>
#include <numeric>
#include <string>
#include <utility>
#include <vector>

export module silicon.json:detail.json_pointer;

import :detail.exceptions;
import :detail.string_concat;
import :detail.string_escape;
import :detail.value_t;


SILICON_JSON_NAMESPACE_BEGIN



export template<typename RefStringType>
class json_pointer {

    silicon_BASIC_JSON_TPL_DECLARATION friend class basic_json;

    template<typename>
    friend class json_pointer;

    template<typename T>
    struct string_t_helper {
        using type = T;
    };

    silicon_BASIC_JSON_TPL_DECLARATION struct string_t_helper<silicon_BASIC_JSON_TPL> {
        using type = StringType;
    };

  public:

    using string_t = typename string_t_helper<RefStringType>::type;



    explicit json_pointer(const string_t &s = "")
        : reference_tokens(split(s)) {}



    string_t to_string() const {
        return std::accumulate(reference_tokens.begin(), reference_tokens.end(), string_t{}, [](const string_t &a, const string_t &b) {
            return detail::concat(a, '/', detail::escape(b));
        });
    }



    JSON_HEDLEY_DEPRECATED_FOR(3.11.0, to_string())
    operator string_t() const {
        return to_string();
    }

#ifndef JSON_NO_IO


    friend std::ostream &operator<<(std::ostream &o, const json_pointer &ptr) {
        o << ptr.to_string();
        return o;
    }
#endif



    json_pointer &operator/=(const json_pointer &ptr) {
        reference_tokens.insert(reference_tokens.end(), ptr.reference_tokens.begin(), ptr.reference_tokens.end());
        return *this;
    }



    json_pointer &operator/=(string_t token) {
        push_back(std::move(token));
        return *this;
    }



    json_pointer &operator/=(std::size_t array_idx) {
        return *this /= std::to_string(array_idx);
    }



    friend json_pointer operator/(const json_pointer &lhs, const json_pointer &rhs) {
        return json_pointer(lhs) /= rhs;
    }



    friend json_pointer operator/(const json_pointer &lhs, string_t token)
    {
        return json_pointer(lhs) /= std::move(token);
    }



    friend json_pointer operator/(const json_pointer &lhs, std::size_t array_idx) {
        return json_pointer(lhs) /= array_idx;
    }



    json_pointer parent_pointer() const {
        if(empty()) {
            return *this;
        }

        json_pointer res = *this;
        res.pop_back();
        return res;
    }



    void pop_back() {
        if(JSON_HEDLEY_UNLIKELY(empty())) {
            JSON_THROW(detail::out_of_range::create(405, "JSON pointer has no parent", nullptr));
        }

        reference_tokens.pop_back();
    }



    const string_t &back() const {
        if(JSON_HEDLEY_UNLIKELY(empty())) {
            JSON_THROW(detail::out_of_range::create(405, "JSON pointer has no parent", nullptr));
        }

        return reference_tokens.back();
    }



    void push_back(const string_t &token) {
        reference_tokens.push_back(token);
    }



    void push_back(string_t &&token) {
        reference_tokens.push_back(std::move(token));
    }



    bool empty() const noexcept {
        return reference_tokens.empty();
    }

  private:
    
    template<typename BasicJsonType>
    static typename BasicJsonType::size_type array_index(const string_t &s) {
        using size_type = typename BasicJsonType::size_type;


        if(JSON_HEDLEY_UNLIKELY(s.size() > 1 && s[0] == '0')) {
            JSON_THROW(detail::parse_error::create(106, 0, detail::concat("array index '", s, "' must not begin with '0'"), nullptr));
        }


        if(JSON_HEDLEY_UNLIKELY(s.size() > 1 && !(s[0] >= '1' && s[0] <= '9'))) {
            JSON_THROW(detail::parse_error::create(109, 0, detail::concat("array index '", s, "' is not a number"), nullptr));
        }

        const char *p = s.c_str();
        char *p_end = nullptr;
        errno = 0;
        const unsigned long long res = std::strtoull(p, &p_end, 10);
        if(p == p_end
           || errno == ERANGE
           || JSON_HEDLEY_UNLIKELY(static_cast<std::size_t>(p_end - p) != s.size()))
        {
            JSON_THROW(detail::out_of_range::create(404, detail::concat("unresolved reference token '", s, "'"), nullptr));
        }



        if(res >= static_cast<unsigned long long>((std::numeric_limits<size_type>::max)()))
        {
            JSON_THROW(detail::out_of_range::create(410, detail::concat("array index ", s, " exceeds size_type"), nullptr));
        }

        return static_cast<size_type>(res);
    }

    JSON_PRIVATE_UNLESS_TESTED: json_pointer top() const {
        if(JSON_HEDLEY_UNLIKELY(empty())) {
            JSON_THROW(detail::out_of_range::create(405, "JSON pointer has no parent", nullptr));
        }

        json_pointer result = *this;
        result.reference_tokens = {reference_tokens[0]};
        return result;
    }

  private:
    
    template<typename BasicJsonType>
    BasicJsonType &get_and_create(BasicJsonType &j) const {
        auto *result = &j;



        for(const auto &reference_token: reference_tokens) {
            switch(result->type()) {
                case detail::value_t::null: {
                    if(reference_token == "0") {

                        result = &result->operator[](0);
                    } else {

                        result = &result->operator[](reference_token);
                    }
                    break;
                }

                case detail::value_t::object: {

                    result = &result->operator[](reference_token);
                    break;
                }

                case detail::value_t::array: {

                    result = &result->operator[](array_index<BasicJsonType>(reference_token));
                    break;
                }

                
                case detail::value_t::string:
                case detail::value_t::boolean:
                case detail::value_t::number_integer:
                case detail::value_t::number_unsigned:
                case detail::value_t::number_float:
                case detail::value_t::binary:
                case detail::value_t::discarded:
                default:
                    JSON_THROW(detail::type_error::create(313, "invalid value to unflatten", &j));
            }
        }

        return *result;
    }

    
    template<typename BasicJsonType>
    BasicJsonType &get_unchecked(BasicJsonType *ptr) const {
        for(const auto &reference_token: reference_tokens) {

            if(ptr->is_null()) {

                const bool nums =
                        std::all_of(reference_token.begin(), reference_token.end(), [](const unsigned char x) {
                            return std::isdigit(x);
                        });


                *ptr = (nums || reference_token == "-")
                               ? detail::value_t::array
                               : detail::value_t::object;
            }

            switch(ptr->type()) {
                case detail::value_t::object: {

                    ptr = &ptr->operator[](reference_token);
                    break;
                }

                case detail::value_t::array: {
                    if(reference_token == "-") {

                        ptr = &ptr->operator[](ptr->m_data.m_value.array->size());
                    } else {

                        ptr = &ptr->operator[](array_index<BasicJsonType>(reference_token));
                    }
                    break;
                }

                case detail::value_t::null:
                case detail::value_t::string:
                case detail::value_t::boolean:
                case detail::value_t::number_integer:
                case detail::value_t::number_unsigned:
                case detail::value_t::number_float:
                case detail::value_t::binary:
                case detail::value_t::discarded:
                default:
                    JSON_THROW(detail::out_of_range::create(404, detail::concat("unresolved reference token '", reference_token, "'"), ptr));
            }
        }

        return *ptr;
    }

    
    template<typename BasicJsonType>
    BasicJsonType &get_checked(BasicJsonType *ptr) const {
        for(const auto &reference_token: reference_tokens) {
            switch(ptr->type()) {
                case detail::value_t::object: {

                    ptr = &ptr->at(reference_token);
                    break;
                }

                case detail::value_t::array: {
                    if(JSON_HEDLEY_UNLIKELY(reference_token == "-")) {

                        JSON_THROW(detail::out_of_range::create(402, detail::concat("array index '-' (", std::to_string(ptr->m_data.m_value.array->size()), ") is out of range"), ptr));
                    }


                    ptr = &ptr->at(array_index<BasicJsonType>(reference_token));
                    break;
                }

                case detail::value_t::null:
                case detail::value_t::string:
                case detail::value_t::boolean:
                case detail::value_t::number_integer:
                case detail::value_t::number_unsigned:
                case detail::value_t::number_float:
                case detail::value_t::binary:
                case detail::value_t::discarded:
                default:
                    JSON_THROW(detail::out_of_range::create(404, detail::concat("unresolved reference token '", reference_token, "'"), ptr));
            }
        }

        return *ptr;
    }

    
    template<typename BasicJsonType>
    const BasicJsonType &get_unchecked(const BasicJsonType *ptr) const {
        for(const auto &reference_token: reference_tokens) {
            switch(ptr->type()) {
                case detail::value_t::object: {

                    ptr = &ptr->operator[](reference_token);
                    break;
                }

                case detail::value_t::array: {
                    if(JSON_HEDLEY_UNLIKELY(reference_token == "-")) {

                        JSON_THROW(detail::out_of_range::create(402, detail::concat("array index '-' (", std::to_string(ptr->m_data.m_value.array->size()), ") is out of range"), ptr));
                    }


                    ptr = &ptr->operator[](array_index<BasicJsonType>(reference_token));
                    break;
                }

                case detail::value_t::null:
                case detail::value_t::string:
                case detail::value_t::boolean:
                case detail::value_t::number_integer:
                case detail::value_t::number_unsigned:
                case detail::value_t::number_float:
                case detail::value_t::binary:
                case detail::value_t::discarded:
                default:
                    JSON_THROW(detail::out_of_range::create(404, detail::concat("unresolved reference token '", reference_token, "'"), ptr));
            }
        }

        return *ptr;
    }

    
    template<typename BasicJsonType>
    const BasicJsonType &get_checked(const BasicJsonType *ptr) const {
        for(const auto &reference_token: reference_tokens) {
            switch(ptr->type()) {
                case detail::value_t::object: {

                    ptr = &ptr->at(reference_token);
                    break;
                }

                case detail::value_t::array: {
                    if(JSON_HEDLEY_UNLIKELY(reference_token == "-")) {

                        JSON_THROW(detail::out_of_range::create(402, detail::concat("array index '-' (", std::to_string(ptr->m_data.m_value.array->size()), ") is out of range"), ptr));
                    }


                    ptr = &ptr->at(array_index<BasicJsonType>(reference_token));
                    break;
                }

                case detail::value_t::null:
                case detail::value_t::string:
                case detail::value_t::boolean:
                case detail::value_t::number_integer:
                case detail::value_t::number_unsigned:
                case detail::value_t::number_float:
                case detail::value_t::binary:
                case detail::value_t::discarded:
                default:
                    JSON_THROW(detail::out_of_range::create(404, detail::concat("unresolved reference token '", reference_token, "'"), ptr));
            }
        }

        return *ptr;
    }

    
    template<typename BasicJsonType>
    bool contains(const BasicJsonType *ptr) const {
        for(const auto &reference_token: reference_tokens) {
            switch(ptr->type()) {
                case detail::value_t::object: {
                    if(!ptr->contains(reference_token)) {

                        return false;
                    }

                    ptr = &ptr->operator[](reference_token);
                    break;
                }

                case detail::value_t::array: {
                    if(JSON_HEDLEY_UNLIKELY(reference_token == "-")) {

                        return false;
                    }
                    if(JSON_HEDLEY_UNLIKELY(reference_token.size() == 1 && !("0" <= reference_token && reference_token <= "9"))) {

                        return false;
                    }
                    if(JSON_HEDLEY_UNLIKELY(reference_token.size() > 1)) {
                        if(JSON_HEDLEY_UNLIKELY(!('1' <= reference_token[0] && reference_token[0] <= '9'))) {

                            return false;
                        }
                        for(std::size_t i = 1; i < reference_token.size(); i++) {
                            if(JSON_HEDLEY_UNLIKELY(!('0' <= reference_token[i] && reference_token[i] <= '9'))) {

                                return false;
                            }
                        }
                    }

                    const auto idx = array_index<BasicJsonType>(reference_token);
                    if(idx >= ptr->size()) {

                        return false;
                    }

                    ptr = &ptr->operator[](idx);
                    break;
                }

                case detail::value_t::null:
                case detail::value_t::string:
                case detail::value_t::boolean:
                case detail::value_t::number_integer:
                case detail::value_t::number_unsigned:
                case detail::value_t::number_float:
                case detail::value_t::binary:
                case detail::value_t::discarded:
                default: {


                    return false;
                }
            }
        }


        return true;
    }

    
    static std::vector<string_t> split(const string_t &reference_string) {
        std::vector<string_t> result;


        if(reference_string.empty()) {
            return result;
        }


        if(JSON_HEDLEY_UNLIKELY(reference_string[0] != '/')) {
            JSON_THROW(detail::parse_error::create(107, 1, detail::concat("JSON pointer must be empty or begin with '/' - was: '", reference_string, "'"), nullptr));
        }




        for(

                std::size_t slash = reference_string.find_first_of('/', 1),

                start = 1;

                start != 0;


                start = (slash == string_t::npos) ? 0 : slash + 1,

                slash = reference_string.find_first_of('/', start)) {


            auto reference_token = reference_string.substr(start, slash - start);


            for(std::size_t pos = reference_token.find_first_of('~');
                pos != string_t::npos;
                pos = reference_token.find_first_of('~', pos + 1)) {
                JSON_ASSERT(reference_token[pos] == '~');


                if(JSON_HEDLEY_UNLIKELY(pos == reference_token.size() - 1 || (reference_token[pos + 1] != '0' && reference_token[pos + 1] != '1'))) {
                    JSON_THROW(detail::parse_error::create(108, 0, "escape character '~' must be followed with '0' or '1'", nullptr));
                }
            }


            detail::unescape(reference_token);
            result.push_back(reference_token);
        }

        return result;
    }

  private:
    
    template<typename BasicJsonType>
    static void flatten(const string_t &reference_string, const BasicJsonType &value, BasicJsonType &result) {
        switch(value.type()) {
            case detail::value_t::array: {
                if(value.m_data.m_value.array->empty()) {

                    result[reference_string] = nullptr;
                } else {

                    for(std::size_t i = 0; i < value.m_data.m_value.array->size(); ++i) {
                        flatten(detail::concat(reference_string, '/', std::to_string(i)),
                                value.m_data.m_value.array->operator[](i), result);
                    }
                }
                break;
            }

            case detail::value_t::object: {
                if(value.m_data.m_value.object->empty()) {

                    result[reference_string] = nullptr;
                } else {

                    for(const auto &element: *value.m_data.m_value.object) {
                        flatten(detail::concat(reference_string, '/', detail::escape(element.first)), element.second, result);
                    }
                }
                break;
            }

            case detail::value_t::null:
            case detail::value_t::string:
            case detail::value_t::boolean:
            case detail::value_t::number_integer:
            case detail::value_t::number_unsigned:
            case detail::value_t::number_float:
            case detail::value_t::binary:
            case detail::value_t::discarded:
            default: {

                result[reference_string] = value;
                break;
            }
        }
    }

    
    template<typename BasicJsonType>
    static BasicJsonType
    unflatten(const BasicJsonType &value) {
        if(JSON_HEDLEY_UNLIKELY(!value.is_object())) {
            JSON_THROW(detail::type_error::create(314, "only objects can be unflattened", &value));
        }

        BasicJsonType result;


        for(const auto &element: *value.m_data.m_value.object) {
            if(JSON_HEDLEY_UNLIKELY(!element.second.is_primitive())) {
                JSON_THROW(detail::type_error::create(315, "values in object must be primitive", &element.second));
            }





            json_pointer(element.first).get_and_create(result) = element.second;
        }

        return result;
    }


    json_pointer<string_t> convert() const & {
        json_pointer<string_t> result;
        result.reference_tokens = reference_tokens;
        return result;
    }

    json_pointer<string_t> convert() && {
        json_pointer<string_t> result;
        result.reference_tokens = std::move(reference_tokens);
        return result;
    }

  public:
#if JSON_HAS_THREE_WAY_COMPARISON


    template<typename RefStringTypeRhs>
    bool operator==(const json_pointer<RefStringTypeRhs> &rhs) const noexcept {
        return reference_tokens == rhs.reference_tokens;
    }



    JSON_HEDLEY_DEPRECATED_FOR(3.11.2, operator==(json_pointer))
    bool operator==(const string_t &rhs) const {
        return *this == json_pointer(rhs);
    }


    template<typename RefStringTypeRhs>
    std::strong_ordering operator<=>(const json_pointer<RefStringTypeRhs> &rhs) const noexcept
    {
        return reference_tokens <=> rhs.reference_tokens;
    }
#else


    template<typename RefStringTypeLhs, typename RefStringTypeRhs>

    friend bool operator==(const json_pointer<RefStringTypeLhs> &lhs, const json_pointer<RefStringTypeRhs> &rhs) noexcept;



    template<typename RefStringTypeLhs, typename StringType>

    friend bool operator==(const json_pointer<RefStringTypeLhs> &lhs, const StringType &rhs);



    template<typename RefStringTypeRhs, typename StringType>

    friend bool operator==(const StringType &lhs, const json_pointer<RefStringTypeRhs> &rhs);



    template<typename RefStringTypeLhs, typename RefStringTypeRhs>

    friend bool operator!=(const json_pointer<RefStringTypeLhs> &lhs, const json_pointer<RefStringTypeRhs> &rhs) noexcept;



    template<typename RefStringTypeLhs, typename StringType>

    friend bool operator!=(const json_pointer<RefStringTypeLhs> &lhs, const StringType &rhs);



    template<typename RefStringTypeRhs, typename StringType>

    friend bool operator!=(const StringType &lhs, const json_pointer<RefStringTypeRhs> &rhs);


    template<typename RefStringTypeLhs, typename RefStringTypeRhs>

    friend bool operator<(const json_pointer<RefStringTypeLhs> &lhs, const json_pointer<RefStringTypeRhs> &rhs) noexcept;
#endif

  private:

    std::vector<string_t> reference_tokens;
};

#if !JSON_HAS_THREE_WAY_COMPARISON

export template<typename RefStringTypeLhs, typename RefStringTypeRhs>
inline bool operator==(const json_pointer<RefStringTypeLhs> &lhs, const json_pointer<RefStringTypeRhs> &rhs) noexcept {
    return lhs.reference_tokens == rhs.reference_tokens;
}

export template<typename RefStringTypeLhs, typename StringType = typename json_pointer<RefStringTypeLhs>::string_t>
JSON_HEDLEY_DEPRECATED_FOR(3.11.2, operator==(json_pointer, json_pointer))
inline bool operator==(const json_pointer<RefStringTypeLhs> &lhs, const StringType &rhs) {
    return lhs == json_pointer<RefStringTypeLhs>(rhs);
}

export template<typename RefStringTypeRhs, typename StringType = typename json_pointer<RefStringTypeRhs>::string_t>
JSON_HEDLEY_DEPRECATED_FOR(3.11.2, operator==(json_pointer, json_pointer))
inline bool operator==(const StringType &lhs, const json_pointer<RefStringTypeRhs> &rhs) {
    return json_pointer<RefStringTypeRhs>(lhs) == rhs;
}

export template<typename RefStringTypeLhs, typename RefStringTypeRhs>
inline bool operator!=(const json_pointer<RefStringTypeLhs> &lhs, const json_pointer<RefStringTypeRhs> &rhs) noexcept {
    return !(lhs == rhs);
}

export template<typename RefStringTypeLhs, typename StringType = typename json_pointer<RefStringTypeLhs>::string_t>
JSON_HEDLEY_DEPRECATED_FOR(3.11.2, operator!=(json_pointer, json_pointer))
inline bool operator!=(const json_pointer<RefStringTypeLhs> &lhs, const StringType &rhs) {
    return !(lhs == rhs);
}

export template<typename RefStringTypeRhs, typename StringType = typename json_pointer<RefStringTypeRhs>::string_t>
JSON_HEDLEY_DEPRECATED_FOR(3.11.2, operator!=(json_pointer, json_pointer))
inline bool operator!=(const StringType &lhs, const json_pointer<RefStringTypeRhs> &rhs) {
    return !(lhs == rhs);
}

export template<typename RefStringTypeLhs, typename RefStringTypeRhs>
inline bool operator<(const json_pointer<RefStringTypeLhs> &lhs, const json_pointer<RefStringTypeRhs> &rhs) noexcept {
    return lhs.reference_tokens < rhs.reference_tokens;
}
#endif

SILICON_JSON_NAMESPACE_END

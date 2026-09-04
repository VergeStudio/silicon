












module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <cmath>
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

export module silicon.json:detail.input.parser;

import :detail.exceptions;
import :detail.input.input_adapters;
import :detail.input.json_sax;
import :detail.input.lexer;
import :detail.meta.detected;
import :detail.meta.is_sax;
import :detail.string_concat;
import :detail.value_t;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {




export enum class parse_event_t : std::uint8_t {

    object_start,

    object_end,

    array_start,

    array_end,

    key,

    value
};

export template<typename BasicJsonType>
using parser_callback_t =
        std::function<bool(int , parse_event_t , BasicJsonType & )>;


export template<typename BasicJsonType, typename InputAdapterType>
class parser {
    using number_integer_t = typename BasicJsonType::number_integer_t;
    using number_unsigned_t = typename BasicJsonType::number_unsigned_t;
    using number_float_t = typename BasicJsonType::number_float_t;
    using string_t = typename BasicJsonType::string_t;
    using lexer_t = lexer<BasicJsonType, InputAdapterType>;
    using token_type = typename lexer_t::token_type;

  public:

    explicit parser(InputAdapterType &&adapter, const parser_callback_t<BasicJsonType> cb = nullptr, const bool allow_exceptions_ = true, const bool skip_comments = false)
        : callback(cb), m_lexer(std::move(adapter), skip_comments), allow_exceptions(allow_exceptions_) {

        get_token();
    }

    
    void parse(const bool strict, BasicJsonType &result) {
        if(callback) {
            json_sax_dom_callback_parser<BasicJsonType> sdp(result, callback, allow_exceptions);
            sax_parse_internal(&sdp);


            if(strict && (get_token() != token_type::end_of_input)) {
                sdp.parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::end_of_input, "value"), nullptr));
            }


            if(sdp.is_errored()) {
                result = value_t::discarded;
                return;
            }



            if(result.is_discarded()) {
                result = nullptr;
            }
        } else {
            json_sax_dom_parser<BasicJsonType> sdp(result, allow_exceptions);
            sax_parse_internal(&sdp);


            if(strict && (get_token() != token_type::end_of_input)) {
                sdp.parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::end_of_input, "value"), nullptr));
            }


            if(sdp.is_errored()) {
                result = value_t::discarded;
                return;
            }
        }

        result.assert_invariant();
    }

    
    bool accept(const bool strict = true) {
        json_sax_acceptor<BasicJsonType> sax_acceptor;
        return sax_parse(&sax_acceptor, strict);
    }

    template<typename SAX>
    JSON_HEDLEY_NON_NULL(2)
    bool sax_parse(SAX *sax, const bool strict = true) {
        (void)detail::is_sax_static_asserts<SAX, BasicJsonType>{};
        const bool result = sax_parse_internal(sax);


        if(result && strict && (get_token() != token_type::end_of_input)) {
            return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::end_of_input, "value"), nullptr));
        }

        return result;
    }

  private:
    template<typename SAX>
    JSON_HEDLEY_NON_NULL(2)
    bool sax_parse_internal(SAX *sax) {


        std::vector<bool> states;

        bool skip_to_state_evaluation = false;

        while(true) {
            if(!skip_to_state_evaluation) {

                switch(last_token) {
                    case token_type::begin_object: {
                        if(JSON_HEDLEY_UNLIKELY(!sax->start_object(static_cast<std::size_t>(-1)))) {
                            return false;
                        }


                        if(get_token() == token_type::end_object) {
                            if(JSON_HEDLEY_UNLIKELY(!sax->end_object())) {
                                return false;
                            }
                            break;
                        }


                        if(JSON_HEDLEY_UNLIKELY(last_token != token_type::value_string)) {
                            return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::value_string, "object key"), nullptr));
                        }
                        if(JSON_HEDLEY_UNLIKELY(!sax->key(m_lexer.get_string()))) {
                            return false;
                        }


                        if(JSON_HEDLEY_UNLIKELY(get_token() != token_type::name_separator)) {
                            return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::name_separator, "object separator"), nullptr));
                        }


                        states.push_back(false);


                        get_token();
                        continue;
                    }

                    case token_type::begin_array: {
                        if(JSON_HEDLEY_UNLIKELY(!sax->start_array(static_cast<std::size_t>(-1)))) {
                            return false;
                        }


                        if(get_token() == token_type::end_array) {
                            if(JSON_HEDLEY_UNLIKELY(!sax->end_array())) {
                                return false;
                            }
                            break;
                        }


                        states.push_back(true);


                        continue;
                    }

                    case token_type::value_float: {
                        const auto res = m_lexer.get_number_float();

                        if(JSON_HEDLEY_UNLIKELY(!std::isfinite(res))) {
                            return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), out_of_range::create(406, concat("number overflow parsing '", m_lexer.get_token_string(), '\''), nullptr));
                        }

                        if(JSON_HEDLEY_UNLIKELY(!sax->number_float(res, m_lexer.get_string()))) {
                            return false;
                        }

                        break;
                    }

                    case token_type::literal_false: {
                        if(JSON_HEDLEY_UNLIKELY(!sax->boolean(false))) {
                            return false;
                        }
                        break;
                    }

                    case token_type::literal_null: {
                        if(JSON_HEDLEY_UNLIKELY(!sax->null())) {
                            return false;
                        }
                        break;
                    }

                    case token_type::literal_true: {
                        if(JSON_HEDLEY_UNLIKELY(!sax->boolean(true))) {
                            return false;
                        }
                        break;
                    }

                    case token_type::value_integer: {
                        if(JSON_HEDLEY_UNLIKELY(!sax->number_integer(m_lexer.get_number_integer()))) {
                            return false;
                        }
                        break;
                    }

                    case token_type::value_string: {
                        if(JSON_HEDLEY_UNLIKELY(!sax->string(m_lexer.get_string()))) {
                            return false;
                        }
                        break;
                    }

                    case token_type::value_unsigned: {
                        if(JSON_HEDLEY_UNLIKELY(!sax->number_unsigned(m_lexer.get_number_unsigned()))) {
                            return false;
                        }
                        break;
                    }

                    case token_type::parse_error: {

                        return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::uninitialized, "value"), nullptr));
                    }
                    case token_type::end_of_input: {
                        if(JSON_HEDLEY_UNLIKELY(m_lexer.get_position().chars_read_total == 1)) {
                            return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), "attempting to parse an empty input; check that your input string or stream contains the expected JSON", nullptr));
                        }

                        return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::literal_or_value, "value"), nullptr));
                    }
                    case token_type::uninitialized:
                    case token_type::end_array:
                    case token_type::end_object:
                    case token_type::name_separator:
                    case token_type::value_separator:
                    case token_type::literal_or_value:
                    default:
                    {
                        return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::literal_or_value, "value"), nullptr));
                    }
                }
            } else {
                skip_to_state_evaluation = false;
            }


            if(states.empty()) {

                return true;
            }

            if(states.back())
            {

                if(get_token() == token_type::value_separator) {

                    get_token();
                    continue;
                }


                if(JSON_HEDLEY_LIKELY(last_token == token_type::end_array)) {
                    if(JSON_HEDLEY_UNLIKELY(!sax->end_array())) {
                        return false;
                    }





                    JSON_ASSERT(!states.empty());
                    states.pop_back();
                    skip_to_state_evaluation = true;
                    continue;
                }

                return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::end_array, "array"), nullptr));
            }




            if(get_token() == token_type::value_separator) {

                if(JSON_HEDLEY_UNLIKELY(get_token() != token_type::value_string)) {
                    return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::value_string, "object key"), nullptr));
                }

                if(JSON_HEDLEY_UNLIKELY(!sax->key(m_lexer.get_string()))) {
                    return false;
                }


                if(JSON_HEDLEY_UNLIKELY(get_token() != token_type::name_separator)) {
                    return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::name_separator, "object separator"), nullptr));
                }


                get_token();
                continue;
            }


            if(JSON_HEDLEY_LIKELY(last_token == token_type::end_object)) {
                if(JSON_HEDLEY_UNLIKELY(!sax->end_object())) {
                    return false;
                }





                JSON_ASSERT(!states.empty());
                states.pop_back();
                skip_to_state_evaluation = true;
                continue;
            }

            return sax->parse_error(m_lexer.get_position(), m_lexer.get_token_string(), parse_error::create(101, m_lexer.get_position(), exception_message(token_type::end_object, "object"), nullptr));
        }
    }


    token_type get_token() {
        return last_token = m_lexer.scan();
    }

    std::string exception_message(const token_type expected, const std::string &context) {
        std::string error_msg = "syntax error ";

        if(!context.empty()) {
            error_msg += concat("while parsing ", context, ' ');
        }

        error_msg += "- ";

        if(last_token == token_type::parse_error) {
            error_msg += concat(m_lexer.get_error_message(), "; last read: '", m_lexer.get_token_string(), '\'');
        } else {
            error_msg += concat("unexpected ", lexer_t::token_type_name(last_token));
        }

        if(expected != token_type::uninitialized) {
            error_msg += concat("; expected ", lexer_t::token_type_name(expected));
        }

        return error_msg;
    }

  private:

    const parser_callback_t<BasicJsonType> callback = nullptr;

    token_type last_token = token_type::uninitialized;

    lexer_t m_lexer;

    const bool allow_exceptions = true;
};

}
SILICON_JSON_NAMESPACE_END

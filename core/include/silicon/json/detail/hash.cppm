//     __ _____ _____ _____
//  __|  |   __|     |   | |  silicon JSON
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon
//
// SPDX-FileCopyrightText: silicon contributors
// SPDX-License-Identifier: MIT

// Partition of the silicon.json module. Macros (JSON_* feature
// flags, SILICON_JSON_NAMESPACE_* ) are NOT exported by C++20
// modules, so the macro headers are textually included in the
// global module fragment of every partition that needs them.

module;

#include <silicon/json/detail/abi_macros.h>
#include <silicon/json/detail/macro_scope.h>
#include <cstddef>    // size_t
#include <cstdint>    // uint8_t
#include <functional> // hash

export module silicon.json:detail.hash;

import :detail.value_t;


SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

// boost::hash_combine
export inline std::size_t combine(std::size_t seed, std::size_t h) noexcept {
    seed ^= h + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
    return seed;
}

/*!
@brief hash a JSON value

The hash function tries to rely on std::hash where possible. Furthermore, the
type of the JSON value is taken into account to have different hash values for
null, 0, 0U, and false, etc.

@tparam BasicJsonType basic_json specialization
@param j JSON value to hash
@return hash value of j
*/
export template<typename BasicJsonType>
std::size_t hash(const BasicJsonType &j) {
    using string_t = typename BasicJsonType::string_t;
    using number_integer_t = typename BasicJsonType::number_integer_t;
    using number_unsigned_t = typename BasicJsonType::number_unsigned_t;
    using number_float_t = typename BasicJsonType::number_float_t;

    const auto type = static_cast<std::size_t>(j.type());
    switch(j.type()) {
        case BasicJsonType::value_t::null:
        case BasicJsonType::value_t::discarded: {
            return combine(type, 0);
        }

        case BasicJsonType::value_t::object: {
            auto seed = combine(type, j.size());
            for(const auto &element: j.items()) {
                const auto h = std::hash<string_t>{}(element.key());
                seed = combine(seed, h);
                seed = combine(seed, hash(element.value()));
            }
            return seed;
        }

        case BasicJsonType::value_t::array: {
            auto seed = combine(type, j.size());
            for(const auto &element: j) {
                seed = combine(seed, hash(element));
            }
            return seed;
        }

        case BasicJsonType::value_t::string: {
            const auto h = std::hash<string_t>{}(j.template get_ref<const string_t &>());
            return combine(type, h);
        }

        case BasicJsonType::value_t::boolean: {
            const auto h = std::hash<bool>{}(j.template get<bool>());
            return combine(type, h);
        }

        case BasicJsonType::value_t::number_integer: {
            const auto h = std::hash<number_integer_t>{}(j.template get<number_integer_t>());
            return combine(type, h);
        }

        case BasicJsonType::value_t::number_unsigned: {
            const auto h = std::hash<number_unsigned_t>{}(j.template get<number_unsigned_t>());
            return combine(type, h);
        }

        case BasicJsonType::value_t::number_float: {
            const auto h = std::hash<number_float_t>{}(j.template get<number_float_t>());
            return combine(type, h);
        }

        case BasicJsonType::value_t::binary: {
            auto seed = combine(type, j.get_binary().size());
            const auto h = std::hash<bool>{}(j.get_binary().has_subtype());
            seed = combine(seed, h);
            seed = combine(seed, static_cast<std::size_t>(j.get_binary().subtype()));
            for(const auto byte: j.get_binary()) {
                seed = combine(seed, std::hash<std::uint8_t>{}(byte));
            }
            return seed;
        }

        default:                // LCOV_EXCL_LINE
            JSON_ASSERT(false); // NOLINT(cert-dcl03-c,hicpp-static-assert,misc-static-assert) LCOV_EXCL_LINE
            return 0;           // LCOV_EXCL_LINE
    }
}

} // namespace detail
SILICON_JSON_NAMESPACE_END

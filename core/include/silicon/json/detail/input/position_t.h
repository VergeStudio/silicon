//     __ _____ _____ _____
//  __|  |   __|     |   | |  silicon JSON
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon
//
// SPDX-FileCopyrightText: silicon contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef> // size_t
#include <silicon/json/detail/abi_macros.h>

SILICON_JSON_NAMESPACE_BEGIN
namespace detail {

/// struct to capture the start position of the current token
struct position_t {
    /// the total number of characters read
    std::size_t chars_read_total = 0;
    /// the number of characters read in the current line
    std::size_t chars_read_current_line = 0;
    /// the number of lines read
    std::size_t lines_read = 0;

    /// conversion to size_t to preserve SAX interface
    constexpr operator size_t() const {
        return chars_read_total;
    }
};

} // namespace detail
SILICON_JSON_NAMESPACE_END

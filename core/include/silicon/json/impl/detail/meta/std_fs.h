//     __ _____ _____ _____
//  __|  |   __|     |   | |  silicon JSON
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon
//
// SPDX-FileCopyrightText: silicon contributors
// SPDX-License-Identifier: MIT

#pragma once

#include <silicon/json/impl/detail/macro_scope.h>

#if JSON_HAS_EXPERIMENTAL_FILESYSTEM
#    include <experimental/filesystem>
SILICON_JSON_NAMESPACE_BEGIN
namespace detail {
namespace std_fs = std::experimental::filesystem;
} // namespace detail
SILICON_JSON_NAMESPACE_END
#elif JSON_HAS_FILESYSTEM
#    include <filesystem>
SILICON_JSON_NAMESPACE_BEGIN
namespace detail {
namespace std_fs = std::filesystem;
} // namespace detail
SILICON_JSON_NAMESPACE_END
#endif

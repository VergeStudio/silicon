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
#if JSON_HAS_EXPERIMENTAL_FILESYSTEM
#    include <experimental/filesystem>
#elif JSON_HAS_FILESYSTEM
#    include <filesystem>
#endif

export module silicon.json:detail.meta.std_fs;


#if JSON_HAS_EXPERIMENTAL_FILESYSTEM
SILICON_JSON_NAMESPACE_BEGIN
namespace detail {
namespace std_fs = std::experimental::filesystem;
} // namespace detail
SILICON_JSON_NAMESPACE_END
#elif JSON_HAS_FILESYSTEM
SILICON_JSON_NAMESPACE_BEGIN
namespace detail {
namespace std_fs = std::filesystem;
} // namespace detail
SILICON_JSON_NAMESPACE_END
#endif

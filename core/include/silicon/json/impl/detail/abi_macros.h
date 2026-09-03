//     __ _____ _____ _____
//  __|  |   __|     |   | |  silicon JSON
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/VergeStudio/silicon
//
// SPDX-FileCopyrightText: silicon contributors
// SPDX-License-Identifier: MIT

#pragma once

// This file contains all macro definitions affecting or depending on the ABI

#ifndef JSON_SKIP_LIBRARY_VERSION_CHECK
#    if defined(SILICON_JSON_VERSION_MAJOR) && defined(SILICON_JSON_VERSION_MINOR) && defined(SILICON_JSON_VERSION_PATCH)
#        if SILICON_JSON_VERSION_MAJOR != 3 || SILICON_JSON_VERSION_MINOR != 11 || SILICON_JSON_VERSION_PATCH != 3
#            warning "Already included a different version of the library!"
#        endif
#    endif
#endif

#define SILICON_JSON_VERSION_MAJOR 3  // NOLINT(modernize-macro-to-enum)
#define SILICON_JSON_VERSION_MINOR 11 // NOLINT(modernize-macro-to-enum)
#define SILICON_JSON_VERSION_PATCH 3  // NOLINT(modernize-macro-to-enum)

#ifndef JSON_DIAGNOSTICS
#    define JSON_DIAGNOSTICS 0
#endif

#ifndef JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON
#    define JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON 0
#endif

#if JSON_DIAGNOSTICS
#    define SILICON_JSON_ABI_TAG_DIAGNOSTICS _diag
#else
#    define SILICON_JSON_ABI_TAG_DIAGNOSTICS
#endif

#if JSON_USE_LEGACY_DISCARDED_VALUE_COMPARISON
#    define SILICON_JSON_ABI_TAG_LEGACY_DISCARDED_VALUE_COMPARISON _ldvcmp
#else
#    define SILICON_JSON_ABI_TAG_LEGACY_DISCARDED_VALUE_COMPARISON
#endif

#ifndef SILICON_JSON_NAMESPACE_NO_VERSION
#    define SILICON_JSON_NAMESPACE_NO_VERSION 0
#endif

// Construct the namespace ABI tags component
#define SILICON_JSON_ABI_TAGS_CONCAT_EX(a, b) json_abi##a##b
#define SILICON_JSON_ABI_TAGS_CONCAT(a, b) \
    SILICON_JSON_ABI_TAGS_CONCAT_EX(a, b)

#define SILICON_JSON_ABI_TAGS                                      \
    SILICON_JSON_ABI_TAGS_CONCAT(                                  \
            SILICON_JSON_ABI_TAG_DIAGNOSTICS,                      \
            SILICON_JSON_ABI_TAG_LEGACY_DISCARDED_VALUE_COMPARISON \
    )

// Construct the namespace version component
#define SILICON_JSON_NAMESPACE_VERSION_CONCAT_EX(major, minor, patch) \
    _v##major##_##minor##_##patch
#define SILICON_JSON_NAMESPACE_VERSION_CONCAT(major, minor, patch) \
    SILICON_JSON_NAMESPACE_VERSION_CONCAT_EX(major, minor, patch)

#if SILICON_JSON_NAMESPACE_NO_VERSION
#    define SILICON_JSON_NAMESPACE_VERSION
#else
#    define SILICON_JSON_NAMESPACE_VERSION \
        SILICON_JSON_NAMESPACE_VERSION_CONCAT(SILICON_JSON_VERSION_MAJOR, SILICON_JSON_VERSION_MINOR, SILICON_JSON_VERSION_PATCH)
#endif

// Combine namespace components
#define SILICON_JSON_NAMESPACE_CONCAT_EX(a, b) a##b
#define SILICON_JSON_NAMESPACE_CONCAT(a, b) \
    SILICON_JSON_NAMESPACE_CONCAT_EX(a, b)

#ifndef SILICON_JSON_NAMESPACE
#    define SILICON_JSON_NAMESPACE                         \
        silicon::json::impl::SILICON_JSON_NAMESPACE_CONCAT( \
                SILICON_JSON_ABI_TAGS,                     \
                SILICON_JSON_NAMESPACE_VERSION             \
        )
#endif

#ifndef SILICON_JSON_NAMESPACE_BEGIN
#    define SILICON_JSON_NAMESPACE_BEGIN                \
        namespace silicon::json::impl {                  \
        inline namespace SILICON_JSON_NAMESPACE_CONCAT( \
                SILICON_JSON_ABI_TAGS,                  \
                SILICON_JSON_NAMESPACE_VERSION          \
        ) {
#endif

#ifndef SILICON_JSON_NAMESPACE_END
#    define SILICON_JSON_NAMESPACE_END                                     \
        } /* namespace (inline namespace) NOLINT(readability/namespace) */ \
        } // namespace silicon::json::impl
#endif

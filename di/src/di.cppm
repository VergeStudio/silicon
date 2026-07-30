module;

// Include all di headers in the global module fragment for template visibility
// This allows the template code to be part of the module.
#include "silicon/di/container.h"
#include "silicon/di/runtime_container.h"
#include "silicon/di/static_container.h"

export module silicon.di;

export import :config;

// The silicon::di library is entirely template-based.
// Key types are re-exported below. For advanced template metaprogramming
// types not listed here, include the specific header directly.

export namespace silicon::di {

using ::silicon::di::runtime_container;
using ::silicon::di::static_container;

// Storage policies
using ::silicon::di::unique;
using ::silicon::di::shared;
using ::silicon::di::external;

// Registration
using ::silicon::di::bind;
using ::silicon::di::type_registration;

// Composition-root primitives（cli 等组合根直接使用）
using ::silicon::di::container;
using ::silicon::di::bindings;
using ::silicon::di::scope;
using ::silicon::di::storage;
using ::silicon::di::interfaces;
using ::silicon::di::factory;

// RTTI
using ::silicon::di::rtti;
using ::silicon::di::type_descriptor;

} // namespace silicon::di

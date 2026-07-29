#pragma once

#include "silicon/di/core/config.h"

namespace silicon::di {
    struct static_provider {};
    struct typeid_provider {};

    template< typename T > class rtti;
}

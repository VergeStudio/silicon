#pragma once

#include "silicon/di/core/config.h"

namespace silicon::di {
class resettable {
  public:
    virtual ~resettable() = default;
    virtual void reset() = 0;
};
} // namespace silicon::di

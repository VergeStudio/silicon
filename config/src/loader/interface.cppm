module;

#include <memory>
#include <string>

#include "silicon/config/common.h"

export module silicon.config.loader.interface;

export namespace silicon::config {

class CONFIG_API ILoader {
  public:
    virtual ~ILoader() = default;
    [[nodiscard]] virtual auto load(const std::string &path) -> bool = 0;
};

} // namespace silicon::config

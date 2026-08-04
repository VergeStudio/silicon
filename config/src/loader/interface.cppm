module;

#include <memory>
#include <string>

#include "silicon/config/common.h"

export module silicon.config.loader.interface;

export namespace silicon::config {

class CONFIG_API Loader {
  public:
    virtual ~Loader() = default;
    [[nodiscard]] virtual auto Load(const std::string &path) -> bool = 0;
};

} // namespace silicon::config

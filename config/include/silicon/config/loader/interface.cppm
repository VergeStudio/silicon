module;

#include <memory>
#include <string>

#include "silicon/config/common.h"

export module silicon.config.loader.interface;

export namespace silicon::config {

class CONFIG_API i_loader {
  public:
    virtual ~i_loader() = default;
    [[nodiscard]] virtual auto Load(const std::string &path) -> bool = 0;
};

} // namespace silicon::config

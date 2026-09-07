module;

#include <memory>
#include <string>
#include <system_error>

#include "silicon/common.h"

export module silicon.event;

export import silicon.event.error;

export namespace silicon::event {

enum class event_status { kSuccess, kFailure, kTimeout };

class SILICON_CORE_API event {
  public:
    event() = default;
    explicit event(std::string) noexcept;
    virtual ~event() = default;

    event(const event &) = delete;
    event(event &&) noexcept = default;
    event & operator=(const event &) = delete;
    event & operator=(event &&) noexcept = default;

    [[nodiscard]] auto name() const noexcept -> const std::string &;
    [[nodiscard]] event_status status() const noexcept ;
    void set_status(event_status) noexcept;

  private:

    struct impl {
        std::string name_;
        event_status status_{event_status::kSuccess};
    };

    std::unique_ptr<impl> impl_{std::make_unique<impl>()};
};

}

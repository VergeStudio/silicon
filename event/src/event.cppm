module;

#include <memory>
#include <string>

#include "silicon/event/common.h"

export module silicon.event;

export import :config;

export namespace silicon::event {

enum class EventStatus { Success, Failure, Timeout };

class EVENT_API Event {
  public:
    Event() noexcept = default;
    explicit Event(std::string name) noexcept;
    virtual ~Event() = default;

    Event(const Event &) = delete;
    Event(Event &&) noexcept = default;
    auto operator=(const Event &) -> Event & = delete;
    auto operator=(Event &&) noexcept -> Event & = default;

    [[nodiscard]] auto name() const noexcept -> const std::string &;
    [[nodiscard]] auto status() const noexcept -> EventStatus;
    void set_status(EventStatus s) noexcept;

  private:
    std::string m_name;
    EventStatus m_status{EventStatus::Success};
};

} // namespace silicon::event

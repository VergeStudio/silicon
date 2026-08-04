module;

#include <memory>
#include <string>

#include "silicon/event/common.h"

export module silicon.event;

export import :config;

export namespace silicon::event {

enum class EventStatus { kSuccess, kFailure, kTimeout };

class EVENT_API Event {
  public:
    Event() = default;
    explicit Event(std::string name) noexcept;
    virtual ~Event() = default;

    Event(const Event &) = delete;
    Event(Event &&) noexcept = default;
    auto operator=(const Event &) -> Event & = delete;
    auto operator=(Event &&) noexcept -> Event & = default;

    [[nodiscard]] auto name() const noexcept -> const std::string &;
    [[nodiscard]] auto status() const noexcept -> EventStatus;
    void SetStatus(EventStatus s) noexcept;

  private:
    // PIMPL：私有状态移入不透明 Impl，稳定 ABI、隐藏实现。
    struct Impl {
        std::string name_;
        EventStatus status_{EventStatus::kSuccess};
    };

    std::unique_ptr<Impl> impl_{std::make_unique<Impl>()};
};

} // namespace silicon::event

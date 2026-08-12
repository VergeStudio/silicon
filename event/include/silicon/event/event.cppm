module;

#include <memory>
#include <string>
#include <system_error>

#include "silicon/event/common.h"

export module silicon.event;

export import :config;

export namespace silicon::event {

/// event 模块专属错误码枚举。
enum class event_error {
    kInvalidStatus = 1,
};

/// 返回 event_error 专属 error_category（name() = "silicon.event"）。
[[nodiscard]] inline const std::error_category &event_category() noexcept {
    static const class : public std::error_category {
        const char *name() const noexcept override { return "silicon.event"; }
        std::string message(int ev) const override {
            switch(static_cast<event_error>(ev)) {
                case event_error::kInvalidStatus: return "invalid event status";
            }
            return "unknown event error";
        }
    } cat;
    return cat;
}

/// 将 event_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(event_error e) noexcept {
    return {static_cast<int>(e), event_category()};
}

enum class event_status { kSuccess, kFailure, kTimeout };

class EVENT_API event {
  public:
    event() = default;
    explicit event(std::string name) noexcept;
    virtual ~event() = default;

    event(const event &) = delete;
    event(event &&) noexcept = default;
    auto operator=(const event &) -> event & = delete;
    auto operator=(event &&) noexcept -> event & = default;

    [[nodiscard]] auto name() const noexcept -> const std::string &;
    [[nodiscard]] auto status() const noexcept -> event_status;
    void set_status(event_status s) noexcept;

  private:
    // PIMPL：私有状态移入不透明 Impl，稳定 ABI、隐藏实现。
    struct Impl {
        std::string name_;
        event_status status_{event_status::kSuccess};
    };

    std::unique_ptr<Impl> impl_{std::make_unique<Impl>()};
};

} // namespace silicon::event

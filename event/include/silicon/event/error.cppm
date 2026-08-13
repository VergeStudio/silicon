module;

#include <string>
#include <system_error>

export module silicon.event.error;

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

} // namespace silicon::event

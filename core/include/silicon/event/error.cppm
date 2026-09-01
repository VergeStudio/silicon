module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>
#include <silicon/common.h>

export module silicon.event.error;

import silicon.error;

export namespace silicon::event {

// 错误类别实例（组合根注入）：跨 DLL 消费方经导出的内联函数 event_category() /
// make_error_code() 引用，须以 EVENT_API 显式导出，否则 LNK2001。
EVENT_API std::atomic<const std::error_category *> event_error_category_instance{nullptr};

/// event 模块专属错误码枚举。
enum class event_error {
    kInvalidStatus = 1,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
// vtable 须以 EVENT_API 显式导出，否则跨 DLL 消费方出现 LNK2001。
class EVENT_API event_category_impl final : public std::error_category {
    const char *name() const noexcept override { return "silicon.event"; }
    std::string message(int ev) const override {
        switch(static_cast<event_error>(ev)) {
            case event_error::kInvalidStatus: return "invalid event status";
        }
        return "unknown event error";
    }
};

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_event_error_category(const std::error_category &cat) noexcept {
    event_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 event_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &event_category() noexcept {
    const std::error_category *cat = event_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// 将 event_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(event_error e) noexcept {
    return {static_cast<int>(e), event_category()};
}

} // namespace silicon::event

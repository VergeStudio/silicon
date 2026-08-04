module;

// 标准库头必须置于全局模块片段：接口单元全局片段中的 #include 对实现单元不可达
#include <string>
#include <utility>

module silicon.event;

namespace silicon::event {

Event::Event(std::string name) noexcept: name_(std::move(name)) {}

auto Event::name() const noexcept -> const std::string & {
    return name_;
}

auto Event::status() const noexcept -> EventStatus {
    return status_;
}

void Event::SetStatus(EventStatus s) noexcept {
    status_ = s;
}

} // namespace silicon::event

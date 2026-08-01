module;

// 标准库头必须置于全局模块片段：接口单元全局片段中的 #include 对实现单元不可达
#include <string>
#include <utility>

module silicon.event;

namespace silicon::event {

Event::Event(std::string name) noexcept {
    m_p->m_name = std::move(name);
}

auto Event::name() const noexcept -> const std::string & {
    return m_p->m_name;
}

auto Event::status() const noexcept -> EventStatus {
    return m_p->m_status;
}

void Event::set_status(EventStatus s) noexcept {
    m_p->m_status = s;
}

} // namespace silicon::event

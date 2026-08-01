#pragma once

#include <memory>
#include <string>

namespace silicon::network {
class hostname {
    struct P {
      public:
        std::string m_hostname;
    };
    std::shared_ptr<P> m_p{std::make_shared<P>()};

  public:
    hostname() = default;
    explicit hostname(std::string hn): m_p(std::make_shared<P>()) { m_p->m_hostname = std::move(hn); }
    // 值类型语义：拷贝做深拷贝，不与源对象共享实现
    hostname(const hostname &o): m_p(std::make_shared<P>(*o.m_p)) {}
    hostname(hostname &&) noexcept = default;
    auto operator=(const hostname &o) -> hostname & {
        if(this != &o) { m_p = std::make_shared<P>(*o.m_p); }
        return *this;
    }
    auto operator=(hostname &&) noexcept -> hostname & = default;
    ~hostname() = default;

    auto data() const -> const std::string & { return m_p->m_hostname; }

    auto operator<=>(const hostname &other) const { return m_p->m_hostname <=> other.m_p->m_hostname; }
    auto operator==(const hostname &other) const -> bool { return m_p->m_hostname == other.m_p->m_hostname; }

  private:
};

} // namespace silicon::network

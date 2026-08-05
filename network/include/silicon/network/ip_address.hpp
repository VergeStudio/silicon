#pragma once

#if defined(_WIN32) || defined(_WIN64)
#    include <winsock2.h>
#    include <ws2tcpip.h>
#else
#    include <arpa/inet.h>
#endif

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>

namespace silicon::network {
enum class domain_t : int {
    kIpv4 = AF_INET,
    kIpv6 = AF_INET6
};

auto to_string(domain_t domain) -> const std::string &;

class ip_address {
  public:
    static const constexpr size_t ipv4_len{4};
    static const constexpr size_t ipv6_len{16};

    ip_address() = default;
    ip_address(std::span<const uint8_t> binary_address, domain_t domain = domain_t::kIpv4): m_p(std::make_shared<P>()) {
        m_p->m_domain = domain;
        if(m_p->m_domain == domain_t::kIpv4 && binary_address.size() > ipv4_len) {
            throw std::runtime_error{"silicon::network::ip_address provided binary ip address is too long"};
        } else if(binary_address.size() > ipv6_len) {
            throw std::runtime_error{"silicon::network::ip_address provided binary ip address is too long"};
        }

        std::copy(binary_address.begin(), binary_address.end(), m_p->m_data.begin());
    }
    // 值类型语义：拷贝做深拷贝，不与源对象共享实现
    ip_address(const ip_address &o): m_p(std::make_shared<P>(*o.m_p)) {}
    ip_address(ip_address &&) noexcept = default;
    auto operator=(const ip_address &o) -> ip_address & {
        if(this != &o) { m_p = std::make_shared<P>(*o.m_p); }
        return *this;
    }
    auto operator=(ip_address &&) noexcept -> ip_address & = default;
    ~ip_address() = default;

    auto domain() const -> domain_t { return m_p->m_domain; }
    auto data() const -> std::span<const uint8_t> {
        if(m_p->m_domain == domain_t::kIpv4) {
            return std::span<const uint8_t>{m_p->m_data.data(), ipv4_len};
        } else {
            return std::span<const uint8_t>{m_p->m_data.data(), ipv6_len};
        }
    }

    static auto from_string(std::string_view address, domain_t domain = domain_t::kIpv4) -> ip_address {
        ip_address addr{};
        addr.m_p->m_domain = domain;

        auto success = inet_pton(static_cast<int>(addr.m_p->m_domain), address.data(), addr.m_p->m_data.data());
        if(success != 1) {
            throw std::runtime_error{"silicon::network::ip_address faild to convert from string"};
        }

        return addr;
    }

    auto to_string() const -> std::string {
        std::string output;
        if(m_p->m_domain == domain_t::kIpv4) {
            output.resize(INET_ADDRSTRLEN, '\0');
        } else {
            output.resize(INET6_ADDRSTRLEN, '\0');
        }

        auto success = inet_ntop(static_cast<int>(m_p->m_domain), m_p->m_data.data(), output.data(), output.length());
        if(success != nullptr) {
            auto len = strnlen(success, output.length());
            output.resize(len);
        } else {
            throw std::runtime_error{"silicon::network::ip_address failed to convert to string representation"};
        }

        return output;
    }

    auto operator<=>(const ip_address &other) const {
        if(auto c = m_p->m_domain <=> other.m_p->m_domain; c != 0) return c;
        return m_p->m_data <=> other.m_p->m_data;
    }
    auto operator==(const ip_address &other) const -> bool {
        return m_p->m_domain == other.m_p->m_domain && m_p->m_data == other.m_p->m_data;
    }

  private:
    struct P {
      public:
        domain_t m_domain{domain_t::kIpv4};
        std::array<uint8_t, ipv6_len> m_data{};
    };
    std::shared_ptr<P> m_p{std::make_shared<P>()};
};

} // namespace silicon::network

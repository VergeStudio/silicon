// 实现单元：silicon.network —— network_error_category 与 make_error_code 定义。
// 每个 category 的 name() 返回模块名，message() 按码值返回人类可读描述。

module;

#include <string>
#include <system_error>

module silicon.network;

namespace silicon::network {
namespace {

class network_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.network"; }
    std::string message(int ev) const override {
        switch(static_cast<network_error>(ev)) {
            case network_error::kUdpNotBound: return "udp socket is not bound";
            case network_error::kCancelled: return "operation cancelled";
            case network_error::kPollingError: return "polling error";
            case network_error::kTimeout: return "operation timed out";
            case network_error::kInvalidIpAddress: return "invalid ip address";
            case network_error::kNullScheduler: return "scheduler must not be null";
            case network_error::kNullExecutor: return "executor must not be null";
            case network_error::kNullTlsContext: return "tls context must not be null";
            case network_error::kSocketCreateFailed: return "failed to create socket";
            case network_error::kSetNonblockingFailed: return "failed to set socket non-blocking";
            case network_error::kSetSockOptFailed: return "failed to set socket option";
            case network_error::kBindFailed: return "failed to bind socket";
            case network_error::kListenFailed: return "failed to listen on socket";
            case network_error::kInvalidSocketType: return "unknown socket type";
            case network_error::kInvalidDomain: return "invalid address domain";
            case network_error::kInvalidConnectStatus: return "invalid connect status value";
            case network_error::kTlsContextInitFailed: return "failed to initialize tls context";
            case network_error::kTlsCertificateLoadFailed: return "failed to load tls certificate";
            case network_error::kTlsPrivateKeyLoadFailed: return "failed to load tls private key";
            case network_error::kTlsKeyMismatch: return "tls certificate and private key do not match";
            case network_error::kDnsInitFailed: return "failed to initialize dns resolver";
            case network_error::kUnknown: return "unknown network error";
        }
        return "unknown network error";
    }
};

} // namespace

const std::error_category &network_category() noexcept {
    static const network_error_category cat{};
    return cat;
}

std::error_code make_error_code(network_error e) noexcept {
    return {static_cast<int>(e), network_category()};
}

} // namespace silicon::network

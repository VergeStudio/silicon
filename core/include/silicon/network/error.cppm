module;

#include <atomic>
#include <exception>
#include <string>
#include <system_error>

#include <silicon/common.h>
export module silicon.network.error;

import silicon.error;

// ---- 模块内部：DI 句柄（不导出）----
namespace silicon::network {

CORE_API std::atomic<const std::error_category *> network_error_category_instance{nullptr};

} // namespace silicon::network

export namespace silicon::network {

/// 网络模块专属错误码枚举。错误码经 make_error_code() 转为 std::error_code
/// （专属 category `silicon.network`）；errno 类错误用 system_error() 助手
/// （定义于 silicon.network:facade 分区）。
enum class network_error {
    kUdpNotBound = 1,
    kCancelled,
    kPollingError,
    kTimeout,
    kInvalidIpAddress,
    // 参数校验（create() 工厂）
    kNullScheduler,
    kNullExecutor,
    kNullTlsContext,
    // socket 操作
    kSocketCreateFailed,
    kSetNonblockingFailed,
    kSetSockOptFailed,
    kBindFailed,
    kListenFailed,
    kInvalidSocketType,
    kInvalidDomain,
    kInvalidConnectStatus,
    // tls
    kTlsContextInitFailed,
    kTlsCertificateLoadFailed,
    kTlsPrivateKeyLoadFailed,
    kTlsKeyMismatch,
    // dns
    kDnsInitFailed,
    kUnknown,
};

// 具名类取代匿名类局部静态（MSVC 模块 vtable 缺陷）；由组合根构造并注入。
class CORE_API network_category_impl final : public std::error_category {
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

/// 组合根注入全局唯一 category 实例（须在任何 make_error_code 调用之前完成）。
inline void inject_network_error_category(const std::error_category &cat) noexcept {
    network_error_category_instance.store(&cat, std::memory_order_release);
}

/// 返回 network_error 专属 error_category。DI 是唯一来源，未注入即终止。
[[nodiscard]] inline const std::error_category &network_category() noexcept {
    const std::error_category *cat = network_error_category_instance.load(std::memory_order_acquire);
    if (cat == nullptr) {
        std::terminate();
    }
    return *cat;
}

/// 将 network_error 转为 std::error_code。
[[nodiscard]] inline std::error_code make_error_code(network_error e) noexcept {
    return {static_cast<int>(e), network_category()};
}

} // namespace silicon::network

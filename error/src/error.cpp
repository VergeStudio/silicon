// 实现单元：silicon.error —— 各模块 error_category 与 make_error_code 定义。
// 每个 category 的 name() 返回模块名，message() 按码值返回人类可读描述。

module;

#include <string>
#include <system_error>

module silicon.error;

namespace silicon::error {
namespace {

// ── core::shared_library ─────────────────────────────────────────
class library_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.library"; }
    std::string message(int ev) const override {
        switch(static_cast<library_error>(ev)) {
            case library_error::kAlreadyLoaded: return "library already loaded";
            case library_error::kLoadFailed: return "failed to load shared library";
            case library_error::kUnloadFailed: return "failed to unload shared library";
            case library_error::kSymbolNotFound: return "symbol not found in shared library";
            case library_error::kInvalidHandle: return "invalid shared library handle";
        }
        return "unknown library error";
    }
};

// ── fs ───────────────────────────────────────────────────────────
class fs_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.fs"; }
    std::string message(int ev) const override {
        switch(static_cast<fs_error>(ev)) {
            case fs_error::kOpenFailed: return "cannot open file";
            case fs_error::kReadFailed: return "cannot read file";
            case fs_error::kWriteFailed: return "cannot write file";
            case fs_error::kNotExist: return "file or directory does not exist";
            case fs_error::kPermissionDenied: return "permission denied";
            case fs_error::kNotDirectory: return "not a directory";
            case fs_error::kAlreadyExists: return "file or directory already exists";
            case fs_error::kUnknown: return "unknown file system error";
        }
        return "unknown fs error";
    }
};

// ── network ──────────────────────────────────────────────────────
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

// ── coroutine::channel ───────────────────────────────────────────
class channel_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.channel"; }
    std::string message(int ev) const override {
        switch(static_cast<channel_error>(ev)) {
            case channel_error::kClosed: return "channel closed";
            case channel_error::kTimeout: return "operation timed out";
            case channel_error::kCancelled: return "operation cancelled";
        }
        return "unknown channel error";
    }
};

// ── coroutine ────────────────────────────────────────────────────
class coroutine_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.coroutine"; }
    std::string message(int ev) const override {
        switch(static_cast<coroutine_error>(ev)) {
            case coroutine_error::kNullExecutor: return "executor must not be null";
            case coroutine_error::kInvalidPoolSize: return "pool size must be greater than zero";
            case coroutine_error::kAlreadyUnlocked: return "mutex is already unlocked";
            case coroutine_error::kUnknown: return "unknown coroutine error";
        }
        return "unknown coroutine error";
    }
};

// ── scheduler ────────────────────────────────────────────────────
class scheduler_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.scheduler"; }
    std::string message(int ev) const override {
        switch(static_cast<scheduler_error>(ev)) {
            case scheduler_error::kShuttingDown: return "scheduler is shutting down";
            case scheduler_error::kResultNotSet:
                return "coroutine result was never set, did you execute the coroutine?";
            case scheduler_error::kInvalidNotifierState: return "invalid io notifier state";
            case scheduler_error::kPipeCreateFailed: return "failed to create pipe";
            case scheduler_error::kEventRegisterFailed: return "failed to register event";
            case scheduler_error::kNullExecutor: return "executor must not be null";
            case scheduler_error::kUnknown: return "unknown scheduler error";
        }
        return "unknown scheduler error";
    }
};

// ── di ───────────────────────────────────────────────────────────
class di_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.di"; }
    std::string message(int ev) const override {
        switch(static_cast<di_error>(ev)) {
            case di_error::kDuplicateBinding: return "duplicate binding";
            case di_error::kUnresolvedDependency: return "unresolved dependency";
            case di_error::kCircularDependency: return "circular dependency detected";
            case di_error::kInvalidType: return "invalid type";
            case di_error::kAlreadyInitialized: return "already initialized";
            case di_error::kTypeNotFound: return "requested type not found in container";
            case di_error::kTypeAmbiguous: return "requested type resolves ambiguously";
            case di_error::kTypeNotConvertible: return "registered type is not convertible to requested type";
            case di_error::kTypeRecursion: return "recursive type resolution detected";
            case di_error::kTypeAlreadyRegistered: return "type already registered";
            case di_error::kTypeIndexAlreadyRegistered: return "type index already registered";
            case di_error::kCollectionTypeNotFound: return "collection element type not found";
            case di_error::kIndexOutOfRange: return "type index out of range";
            case di_error::kUnknown: return "unknown di error";
        }
        return "unknown di error";
    }
};

// ── http ─────────────────────────────────────────────────────────
class http_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.http"; }
    std::string message(int ev) const override {
        switch(static_cast<http_error>(ev)) {
            case http_error::kRequestFailed: return "http request failed";
            case http_error::kInvalidResponse: return "invalid http response";
            case http_error::kTimeout: return "http request timed out";
            case http_error::kUnknown: return "unknown http error";
        }
        return "unknown http error";
    }
};

// ── llm ──────────────────────────────────────────────────────────
class llm_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.llm"; }
    std::string message(int ev) const override {
        switch(static_cast<llm_error>(ev)) {
            case llm_error::kProviderUnavailable: return "llm provider unavailable";
            case llm_error::kInvalidResponse: return "invalid llm response";
            case llm_error::kToolNotFound: return "tool not found";
            case llm_error::kTimeout: return "llm request timed out";
            case llm_error::kUnknown: return "unknown llm error";
        }
        return "unknown llm error";
    }
};

// ── cli ──────────────────────────────────────────────────────────
class cli_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.cli"; }
    std::string message(int ev) const override {
        switch(static_cast<cli_error>(ev)) {
            case cli_error::kParseFailed: return "cli parse failed";
            case cli_error::kUnknownOption: return "unknown option";
            case cli_error::kMissingArgument: return "missing argument";
            case cli_error::kInvalidValue: return "invalid value";
        }
        return "unknown cli error";
    }
};

// ── config ───────────────────────────────────────────────────────
class config_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.config"; }
    std::string message(int ev) const override {
        switch(static_cast<config_error>(ev)) {
            case config_error::kLoadFailed: return "config load failed";
            case config_error::kParseFailed: return "config parse failed";
            case config_error::kInvalidValue: return "invalid config value";
        }
        return "unknown config error";
    }
};

// ── event ────────────────────────────────────────────────────────
class event_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.event"; }
    std::string message(int ev) const override {
        switch(static_cast<event_error>(ev)) {
            case event_error::kInvalidStatus: return "invalid event status";
        }
        return "unknown event error";
    }
};

// ── logger ───────────────────────────────────────────────────────
class logger_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.logger"; }
    std::string message(int ev) const override {
        switch(static_cast<logger_error>(ev)) {
            case logger_error::kInitFailed: return "logger init failed";
            case logger_error::kInvalidLevel: return "invalid log level";
        }
        return "unknown logger error";
    }
};

// ── plugin ───────────────────────────────────────────────────────
class plugin_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.plugin"; }
    std::string message(int ev) const override {
        switch(static_cast<plugin_error>(ev)) {
            case plugin_error::kLoadFailed: return "plugin load failed";
            case plugin_error::kUnloadFailed: return "plugin unload failed";
            case plugin_error::kDuplicate: return "plugin already registered";
            case plugin_error::kNotFound: return "plugin not found";
            case plugin_error::kNullPlugin: return "null plugin handle";
        }
        return "unknown plugin error";
    }
};

// ── tui ──────────────────────────────────────────────────────────
class tui_error_category final : public std::error_category {
  public:
    const char *name() const noexcept override { return "silicon.tui"; }
    std::string message(int ev) const override {
        switch(static_cast<tui_error>(ev)) {
            case tui_error::kInitFailed: return "tui init failed";
            case tui_error::kInvalidTerminal: return "invalid terminal";
        }
        return "unknown tui error";
    }
};

} // namespace

// ── category 访问器 ──────────────────────────────────────────────
const std::error_category &library_category() noexcept {
    static const library_error_category cat{};
    return cat;
}
const std::error_category &fs_category() noexcept {
    static const fs_error_category cat{};
    return cat;
}
const std::error_category &network_category() noexcept {
    static const network_error_category cat{};
    return cat;
}
const std::error_category &channel_category() noexcept {
    static const channel_error_category cat{};
    return cat;
}
const std::error_category &coroutine_category() noexcept {
    static const coroutine_error_category cat{};
    return cat;
}
const std::error_category &scheduler_category() noexcept {
    static const scheduler_error_category cat{};
    return cat;
}
const std::error_category &di_category() noexcept {
    static const di_error_category cat{};
    return cat;
}
const std::error_category &http_category() noexcept {
    static const http_error_category cat{};
    return cat;
}
const std::error_category &llm_category() noexcept {
    static const llm_error_category cat{};
    return cat;
}
const std::error_category &cli_category() noexcept {
    static const cli_error_category cat{};
    return cat;
}
const std::error_category &config_category() noexcept {
    static const config_error_category cat{};
    return cat;
}
const std::error_category &event_category() noexcept {
    static const event_error_category cat{};
    return cat;
}
const std::error_category &logger_category() noexcept {
    static const logger_error_category cat{};
    return cat;
}
const std::error_category &plugin_category() noexcept {
    static const plugin_error_category cat{};
    return cat;
}
const std::error_category &tui_category() noexcept {
    static const tui_error_category cat{};
    return cat;
}

// ── make_error_code ──────────────────────────────────────────────
std::error_code make_error_code(library_error e) noexcept {
    return {static_cast<int>(e), library_category()};
}
std::error_code make_error_code(fs_error e) noexcept {
    return {static_cast<int>(e), fs_category()};
}
std::error_code make_error_code(network_error e) noexcept {
    return {static_cast<int>(e), network_category()};
}
std::error_code make_error_code(channel_error e) noexcept {
    return {static_cast<int>(e), channel_category()};
}
std::error_code make_error_code(coroutine_error e) noexcept {
    return {static_cast<int>(e), coroutine_category()};
}
std::error_code make_error_code(scheduler_error e) noexcept {
    return {static_cast<int>(e), scheduler_category()};
}
std::error_code make_error_code(di_error e) noexcept {
    return {static_cast<int>(e), di_category()};
}
std::error_code make_error_code(http_error e) noexcept {
    return {static_cast<int>(e), http_category()};
}
std::error_code make_error_code(llm_error e) noexcept {
    return {static_cast<int>(e), llm_category()};
}
std::error_code make_error_code(cli_error e) noexcept {
    return {static_cast<int>(e), cli_category()};
}
std::error_code make_error_code(config_error e) noexcept {
    return {static_cast<int>(e), config_category()};
}
std::error_code make_error_code(event_error e) noexcept {
    return {static_cast<int>(e), event_category()};
}
std::error_code make_error_code(logger_error e) noexcept {
    return {static_cast<int>(e), logger_category()};
}
std::error_code make_error_code(plugin_error e) noexcept {
    return {static_cast<int>(e), plugin_category()};
}
std::error_code make_error_code(tui_error e) noexcept {
    return {static_cast<int>(e), tui_category()};
}

} // namespace silicon::error

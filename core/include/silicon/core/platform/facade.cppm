/// @file facade.cppm
/// @brief Platform detection — C++23 modules + constexpr.
/// @usage
///   import silicon.platform;
///   if constexpr (os == os_id::kWindowsNt) { /* Windows */ }
///   if constexpr (os == os_id::kLinuxOs)    { /* Linux  */ }
///   // Endianness: use std::endian::native (C++20)

module;

#include <memory>
#include <string>
#include <tuple>
// proxy dispatch 宏头：宏不随 C++20 模块导出，必须在全局模块片段文本包含
#include <silicon/proxy/proxy_macros.h>
export module silicon.platform;


import silicon.proxy;

export namespace silicon::platform {

// ── OS identity ──────────────────────────────────────────────────────
enum class os_id : unsigned {
    kFreeBsd = 0x0001,
    kAix = 0x0002,
    kHpux = 0x0003,
    kTru64 = 0x0004,
    kLinuxOs = 0x0005,
    kMacOsX = 0x0006,
    kNetBsd = 0x0007,
    kOpenBsd = 0x0008,
    kIrix = 0x0009,
    kSolaris = 0x000a,
    kQnxOs = 0x000b,
    kVxWorks = 0x000c,
    kCygwin = 0x000d,
    kNacl = 0x000e,
    kAndroid = 0x000f,
    kGnuHurd = 0x0010,
    kUnknownUnix = 0x00ff,
    kWindowsNt = 0x1001,
    kVms = 0x2001,
};

// ── Architecture identity ──────────────────────────────────────────────
enum class arch_id : unsigned {
    kAlpha = 0x01,
    kIa32 = 0x02,
    kIa64 = 0x03,
    kMips = 0x04,
    kHppa = 0x05,
    kPpc = 0x06,
    kPower = 0x07,
    kSparc = 0x08,
    kAmd64 = 0x09,
    kArm = 0x0a,
    kM68k = 0x0b,
    kS390 = 0x0c,
    kSh = 0x0d,
    kNios2 = 0x0e,
    kAarch64 = 0x0f,
    kArm64 = 0x0f,
    kRiscv64 = 0x10,
    kRiscv32 = 0x11,
    kLoongarch64 = 0x12,
};

// ── Compile-time detected values ─────────────────────────────────────
// Exactly one branch is active per compilation.
//
// 统一平台守卫：与全项目一致，OS 族判定消费构建系统注入的 `SILICON_PLATFORM_*`
// 宏（根 xmake.lua 按 is_plat() 定义 WINDOWS/UNIX/LINUX/APPLE/BSD；CMakeLists.txt
// 为 Windows-only 路径，恒定义 WINDOWS）。不再裸探测编译器预定义宏，消除本模块
// 与项目其余部分重复探测导致的逻辑分叉。`SILICON_PLATFORM_*` 仅覆盖 OS 族，
// 故架构探测（下方 arch）仍基于编译器预定义宏，二者分工明确。

#if defined(SILICON_PLATFORM_WINDOWS)
constexpr os_id os = os_id::kWindowsNt;
#elif defined(SILICON_PLATFORM_LINUX)
constexpr os_id os = os_id::kLinuxOs;
#elif defined(SILICON_PLATFORM_APPLE)
constexpr os_id os = os_id::kMacOsX;
#elif defined(SILICON_PLATFORM_BSD)
constexpr os_id os = os_id::kFreeBsd;
#elif defined(SILICON_PLATFORM_UNIX)
constexpr os_id os = os_id::kUnknownUnix;
#else
#    error "No SILICON_PLATFORM_* guard defined — verify build-system platform defines."
#endif

#if defined(__ALPHA) || defined(__alpha) || defined(__alpha__) || defined(_M_ALPHA)
constexpr arch_id arch = arch_id::kAlpha;
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(__EMSCRIPTEN__)
constexpr arch_id arch = arch_id::kIa32;
#elif defined(_IA64) || defined(__IA64__) || defined(__ia64__) || defined(_M_IA64)
constexpr arch_id arch = arch_id::kIa64;
#elif defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
constexpr arch_id arch = arch_id::kAmd64;
#elif defined(__aarch64__) || defined(__arm64) || defined(_M_ARM64)
constexpr arch_id arch = arch_id::kAarch64;
#elif defined(__riscv)
#    if __riscv_xlen == 64
constexpr arch_id arch = arch_id::kRiscv64;
#    elif __riscv_xlen == 32
constexpr arch_id arch = arch_id::kRiscv32;
#    endif
#elif defined(__loongarch64)
constexpr arch_id arch = arch_id::kLoongarch64;
#else
#    error "Unknown hardware architecture."
#endif

// ── Helper functions ─────────────────────────────────────────────────
// Family checks (compile-time)

constexpr bool IsUnixFamily(os_id o) {
    return o == os_id::kFreeBsd || o == os_id::kAix ||
           o == os_id::kHpux || o == os_id::kTru64 ||
           o == os_id::kNacl || o == os_id::kLinuxOs ||
           o == os_id::kMacOsX || o == os_id::kNetBsd ||
           o == os_id::kOpenBsd || o == os_id::kIrix ||
           o == os_id::kSolaris || o == os_id::kQnxOs ||
           o == os_id::kCygwin || o == os_id::kNacl ||
           o == os_id::kAndroid || o == os_id::kGnuHurd ||
           o == os_id::kUnknownUnix;
}

constexpr bool IsBsdFamily(os_id o) {
    return o == os_id::kFreeBsd || o == os_id::kNetBsd ||
           o == os_id::kOpenBsd || o == os_id::kMacOsX;
}

constexpr bool IsWindowsFamily(os_id o) {
    return o == os_id::kWindowsNt;
}

// ── i_platform properties (compile-time) ──────────────────────────────

constexpr auto NativeNewline() {
    if constexpr(os == os_id::kWindowsNt)
        return "\r\n";
    else
        return "\n";
}

constexpr auto path_separator() {
    if constexpr(os == os_id::kWindowsNt)
        return "\\";
    else
        return "/";
}

constexpr char PathSeparatorChar() {
    if constexpr(os == os_id::kWindowsNt)
        return ';';
    else
        return ':';
}

constexpr auto shared_lib_prefix() {
    if constexpr(os == os_id::kWindowsNt)
        return "";
    else
        return "lib";
}

constexpr auto shared_lib_suffix() {
    if constexpr(os == os_id::kWindowsNt)
        return ".dll";
    else if constexpr(os == os_id::kMacOsX)
        return ".dylib";
    else if constexpr(os == os_id::kHpux)
        return ".sl";
    else
        return ".so";
}

constexpr auto executable_suffix() {
    if constexpr(os == os_id::kWindowsNt)
        return ".exe";
    else
        return "";
}

// ── Runtime abstraction（silicon.proxy type-erased 门面）──

// 平台门面：鸭子类型满足即可（os_name / path_separator / line_ending 三约定）。
PRO_DEF_MEM_DISPATCH(MemPlatformOsName, os_name);
PRO_DEF_MEM_DISPATCH(MemPlatformPathSeparator, path_separator);
PRO_DEF_MEM_DISPATCH(MemPlatformLineEnding, line_ending);
struct platform_facade: silicon::proxy::facade_builder ::add_convention<MemPlatformOsName, std::string() const>::add_convention<MemPlatformPathSeparator, char() const>::add_convention<MemPlatformLineEnding, std::string() const>::build {};

using platform_proxy = silicon::proxy::proxy<platform_facade>;
using platform_view = silicon::proxy::proxy_view<platform_facade>;

template<class T, class... Args>
[[nodiscard]] platform_proxy make_platform(Args &&...args) {
    return silicon::proxy::make_proxy<platform_facade, T>(
            std::forward<Args>(args)...
    );
}

/// 为已存在的平台对象创建非拥有视图；调用方负责保证生命周期。
template<class T>
    requires silicon::proxy::proxiable_target<T, platform_facade>
[[nodiscard]] platform_view make_platform_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<platform_facade>(target);
}

class windows_platform {
  public:
    std::string os_name() const { return "windows"; }
    char path_separator() const { return '\\'; }
    std::string line_ending() const { return "\r\n"; }
};

class linux_platform {
  public:
    std::string os_name() const { return "linux"; }
    char path_separator() const { return '/'; }
    std::string line_ending() const { return "\n"; }
};

class unix_platform {
  public:
    std::string os_name() const { return "unix"; }
    char path_separator() const { return '/'; }
    std::string line_ending() const { return "\n"; }
};

// 编译期选中当前平台实现（互斥，仅一个分支参与重载决议），返回拥有句柄。
inline platform_proxy create_platform() {
    if constexpr(os == os_id::kWindowsNt) {
        return make_platform<windows_platform>();
    } else if constexpr(os == os_id::kLinuxOs) {
        return make_platform<linux_platform>();
    } else {
        return make_platform<unix_platform>();
    }
}

} // namespace silicon::platform

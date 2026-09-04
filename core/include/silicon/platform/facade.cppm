







module;

#include <memory>
#include <string>
#include <tuple>

#include <silicon/proxy/proxy_macros.h>



#include <silicon/common.h>
export module silicon.platform;


import silicon.proxy;

export namespace silicon::platform {


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


template<class T>
    requires silicon::proxy::proxiable_target<T, platform_facade>
[[nodiscard]] platform_view make_platform_view(T &target) noexcept {
    return silicon::proxy::make_proxy_view<platform_facade>(target);
}



[[nodiscard]] CORE_API platform_proxy create_platform();

}

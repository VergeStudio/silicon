/// @file platform.cppm
/// @brief IPlatform detection — C++23 modules + constexpr.
/// @usage
///   import silicon.platform;
///   if constexpr (os == OsId::kWindowsNt) { /* Windows */ }
///   if constexpr (os == OsId::kLinuxOs)    { /* Linux  */ }
///   // Endianness: use std::endian::native (C++20)

module;

#include <memory>
#include <string>

export module silicon.platform;

export namespace silicon::platform {

    // ── OS identity ──────────────────────────────────────────────────────
    enum class OsId : unsigned {
        kFreeBsd    = 0x0001,
        kAix          = 0x0002,
        kHpux         = 0x0003,
        kTru64        = 0x0004,
        kLinuxOs     = 0x0005,
        kMacOsX     = 0x0006,
        kNetBsd      = 0x0007,
        kOpenBsd     = 0x0008,
        kIrix         = 0x0009,
        kSolaris      = 0x000a,
        kQnxOs       = 0x000b,
        kVxWorks      = 0x000c,
        kCygwin       = 0x000d,
        kNacl         = 0x000e,
        kAndroid      = 0x000f,
        kGnuHurd     = 0x0010,
        kUnknownUnix = 0x00ff,
        kWindowsNt   = 0x1001,
        kVms          = 0x2001,
    };

    // ── Architecture identity ──────────────────────────────────────────────
    enum class ArchId : unsigned {
        kAlpha       = 0x01,
        kIa32        = 0x02,
        kIa64        = 0x03,
        kMips        = 0x04,
        kHppa        = 0x05,
        kPpc         = 0x06,
        kPower       = 0x07,
        kSparc       = 0x08,
        kAmd64       = 0x09,
        kArm         = 0x0a,
        kM68k        = 0x0b,
        kS390        = 0x0c,
        kSh          = 0x0d,
        kNios2       = 0x0e,
        kAarch64     = 0x0f,
        kArm64       = 0x0f,
        kRiscv64     = 0x10,
        kRiscv32     = 0x11,
        kLoongarch64 = 0x12,
    };

    // ── Compile-time detected values ─────────────────────────────────────
    // Exactly one branch is active per compilation.

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    constexpr OsId   os   = OsId::kFreeBsd;
#elif defined(_AIX) || defined(__TOS_AIX__)
    constexpr OsId   os   = OsId::kAix;
#elif defined(hpux) || defined(_hpux) || defined(__hpux)
    constexpr OsId   os   = OsId::kHpux;
#elif defined(__digital__) || defined(__osf__)
    constexpr OsId   os   = OsId::kTru64;
#elif defined(__NACL__)
    constexpr OsId   os   = OsId::kNacl;
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__TOS_LINUX__) || defined(__EMSCRIPTEN__)
    #if defined(__ANDROID__)
        constexpr OsId os = OsId::kAndroid;
    #else
        constexpr OsId os = OsId::kLinuxOs;
    #endif
#elif defined(__APPLE__) || defined(__TOS_MACOS__)
    constexpr OsId   os   = OsId::kMacOsX;
#elif defined(__NetBSD__)
    constexpr OsId   os   = OsId::kNetBsd;
#elif defined(__OpenBSD__)
    constexpr OsId   os   = OsId::kOpenBsd;
#elif defined(sgi) || defined(__sgi)
    constexpr OsId   os   = OsId::kIrix;
#elif defined(sun) || defined(__sun)
    constexpr OsId   os   = OsId::kSolaris;
#elif defined(__QNX__)
    constexpr OsId   os   = OsId::kQnxOs;
#elif defined(__CYGWIN__)
    constexpr OsId   os   = OsId::kCygwin;
#elif defined(__VMS)
    constexpr OsId   os   = OsId::kVms;
#elif defined(unix) || defined(__unix) || defined(__unix__)
    constexpr OsId   os   = OsId::kUnknownUnix;
#elif defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
    constexpr OsId   os   = OsId::kWindowsNt;
#else
    #error "Unknown platform — cannot detect OS."
#endif

#if defined(__ALPHA) || defined(__alpha) || defined(__alpha__) || defined(_M_ALPHA)
    constexpr ArchId arch = ArchId::kAlpha;
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(__EMSCRIPTEN__)
    constexpr ArchId arch = ArchId::kIa32;
#elif defined(_IA64) || defined(__IA64__) || defined(__ia64__) || defined(_M_IA64)
    constexpr ArchId arch = ArchId::kIa64;
#elif defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
    constexpr ArchId arch = ArchId::kAmd64;
#elif defined(__aarch64__) || defined(__arm64) || defined(_M_ARM64)
    constexpr ArchId arch = ArchId::kAarch64;
#elif defined(__riscv)
    #if __riscv_xlen == 64
        constexpr ArchId arch = ArchId::kRiscv64;
    #elif __riscv_xlen == 32
        constexpr ArchId arch = ArchId::kRiscv32;
    #endif
#elif defined(__loongarch64)
    constexpr ArchId arch = ArchId::kLoongarch64;
#else
    #error "Unknown hardware architecture."
#endif

    // ── Helper functions ─────────────────────────────────────────────────
    // Family checks (compile-time)

    constexpr bool IsUnixFamily(OsId o) {
        return o == OsId::kFreeBsd  || o == OsId::kAix        ||
               o == OsId::kHpux      || o == OsId::kTru64      ||
               o == OsId::kNacl      || o == OsId::kLinuxOs   ||
               o == OsId::kMacOsX  || o == OsId::kNetBsd    ||
               o == OsId::kOpenBsd  || o == OsId::kIrix       ||
               o == OsId::kSolaris   || o == OsId::kQnxOs     ||
               o == OsId::kCygwin    || o == OsId::kNacl       ||
               o == OsId::kAndroid   || o == OsId::kGnuHurd   ||
               o == OsId::kUnknownUnix;
    }

    constexpr bool IsBsdFamily(OsId o) {
        return o == OsId::kFreeBsd || o == OsId::kNetBsd ||
               o == OsId::kOpenBsd || o == OsId::kMacOsX;
    }

    constexpr bool IsWindowsFamily(OsId o) {
        return o == OsId::kWindowsNt;
    }

    // ── IPlatform properties (compile-time) ──────────────────────────────

    constexpr auto NativeNewline() {
        if constexpr (os == OsId::kWindowsNt)
            return "\r\n";
        else
            return "\n";
    }

    constexpr auto PathSeparator() {
        if constexpr (os == OsId::kWindowsNt)
            return "\\";
        else
            return "/";
    }

    constexpr char PathSeparatorChar() {
        if constexpr (os == OsId::kWindowsNt)
            return ';';
        else
            return ':';
    }

    constexpr auto SharedLibPrefix() {
        if constexpr (os == OsId::kWindowsNt)
            return "";
        else
            return "lib";
    }

    constexpr auto SharedLibSuffix() {
        if constexpr (os == OsId::kWindowsNt)
            return ".dll";
        else if constexpr (os == OsId::kMacOsX)
            return ".dylib";
        else if constexpr (os == OsId::kHpux)
            return ".sl";
        else
            return ".so";
    }

    constexpr auto ExecutableSuffix() {
        if constexpr (os == OsId::kWindowsNt)
            return ".exe";
        else
            return "";
    }

    // ── Runtime abstraction（spec：IPlatform / 各平台实现 / CreatePlatform） ──

    class IPlatform {
      public:
        virtual ~IPlatform() = default;
        virtual std::string OsName() const = 0;
        virtual char PathSeparator() const = 0;
        virtual std::string LineEnding() const = 0;
    };

    class WindowsPlatform: public IPlatform {
      public:
        std::string OsName() const override { return "windows"; }
        char PathSeparator() const override { return '\\'; }
        std::string LineEnding() const override { return "\r\n"; }
    };

    class LinuxPlatform: public IPlatform {
      public:
        std::string OsName() const override { return "linux"; }
        char PathSeparator() const override { return '/'; }
        std::string LineEnding() const override { return "\n"; }
    };

    class UnixPlatform: public IPlatform {
      public:
        std::string OsName() const override { return "unix"; }
        char PathSeparator() const override { return '/'; }
        std::string LineEnding() const override { return "\n"; }
    };

    // 编译期选中当前平台实现（互斥，仅一个分支参与重载决议）。
    inline std::unique_ptr<IPlatform> CreatePlatform() {
        if constexpr (os == OsId::kWindowsNt) {
            return std::make_unique<WindowsPlatform>();
        } else if constexpr (os == OsId::kLinuxOs) {
            return std::make_unique<LinuxPlatform>();
        } else {
            return std::make_unique<UnixPlatform>();
        }
    }

} // namespace silicon::platform

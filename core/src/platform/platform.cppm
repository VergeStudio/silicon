/// @file platform.cppm
/// @brief Platform detection — C++23 modules + constexpr.
/// @usage
///   import silicon.platform;
///   if constexpr (os == os_id::windows_nt) { /* Windows */ }
///   if constexpr (os == os_id::linux_os)    { /* Linux  */ }
///   // Endianness: use std::endian::native (C++20)

module;

export module silicon.platform;

export namespace silicon::platform {

    // ── OS identity ──────────────────────────────────────────────────────
    enum class os_id : unsigned {
        free_bsd    = 0x0001,
        aix          = 0x0002,
        hpux         = 0x0003,
        tru64        = 0x0004,
        linux_os     = 0x0005,
        mac_os_x     = 0x0006,
        net_bsd      = 0x0007,
        open_bsd     = 0x0008,
        irix         = 0x0009,
        solaris      = 0x000a,
        qnx_os       = 0x000b,
        vxworks      = 0x000c,
        cygwin       = 0x000d,
        nacl         = 0x000e,
        android      = 0x000f,
        gnu_hurd     = 0x0010,
        unknown_unix = 0x00ff,
        windows_nt   = 0x1001,
        vms          = 0x2001,
    };

    // ── Architecture identity ──────────────────────────────────────────────
    enum class arch_id : unsigned {
        alpha       = 0x01,
        ia32        = 0x02,
        ia64        = 0x03,
        mips        = 0x04,
        hppa        = 0x05,
        ppc         = 0x06,
        power       = 0x07,
        sparc       = 0x08,
        amd64       = 0x09,
        arm         = 0x0a,
        m68k        = 0x0b,
        s390        = 0x0c,
        sh          = 0x0d,
        nios2       = 0x0e,
        aarch64     = 0x0f,
        arm64       = 0x0f,
        riscv64     = 0x10,
        riscv32     = 0x11,
        loongarch64 = 0x12,
    };

    // ── Compile-time detected values ─────────────────────────────────────
    // Exactly one branch is active per compilation.

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
    constexpr os_id   os   = os_id::free_bsd;
#elif defined(_AIX) || defined(__TOS_AIX__)
    constexpr os_id   os   = os_id::aix;
#elif defined(hpux) || defined(_hpux) || defined(__hpux)
    constexpr os_id   os   = os_id::hpux;
#elif defined(__digital__) || defined(__osf__)
    constexpr os_id   os   = os_id::tru64;
#elif defined(__NACL__)
    constexpr os_id   os   = os_id::nacl;
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(__TOS_LINUX__) || defined(__EMSCRIPTEN__)
    #if defined(__ANDROID__)
        constexpr os_id os = os_id::android;
    #else
        constexpr os_id os = os_id::linux_os;
    #endif
#elif defined(__APPLE__) || defined(__TOS_MACOS__)
    constexpr os_id   os   = os_id::mac_os_x;
#elif defined(__NetBSD__)
    constexpr os_id   os   = os_id::net_bsd;
#elif defined(__OpenBSD__)
    constexpr os_id   os   = os_id::open_bsd;
#elif defined(sgi) || defined(__sgi)
    constexpr os_id   os   = os_id::irix;
#elif defined(sun) || defined(__sun)
    constexpr os_id   os   = os_id::solaris;
#elif defined(__QNX__)
    constexpr os_id   os   = os_id::qnx_os;
#elif defined(__CYGWIN__)
    constexpr os_id   os   = os_id::cygwin;
#elif defined(__VMS)
    constexpr os_id   os   = os_id::vms;
#elif defined(unix) || defined(__unix) || defined(__unix__)
    constexpr os_id   os   = os_id::unknown_unix;
#elif defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
    constexpr os_id   os   = os_id::windows_nt;
#else
    #error "Unknown platform — cannot detect OS."
#endif

#if defined(__ALPHA) || defined(__alpha) || defined(__alpha__) || defined(_M_ALPHA)
    constexpr arch_id arch = arch_id::alpha;
#elif defined(i386) || defined(__i386) || defined(__i386__) || defined(_M_IX86) || defined(__EMSCRIPTEN__)
    constexpr arch_id arch = arch_id::ia32;
#elif defined(_IA64) || defined(__IA64__) || defined(__ia64__) || defined(_M_IA64)
    constexpr arch_id arch = arch_id::ia64;
#elif defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64)
    constexpr arch_id arch = arch_id::amd64;
#elif defined(__aarch64__) || defined(__arm64) || defined(_M_ARM64)
    constexpr arch_id arch = arch_id::aarch64;
#elif defined(__riscv)
    #if __riscv_xlen == 64
        constexpr arch_id arch = arch_id::riscv64;
    #elif __riscv_xlen == 32
        constexpr arch_id arch = arch_id::riscv32;
    #endif
#elif defined(__loongarch64)
    constexpr arch_id arch = arch_id::loongarch64;
#else
    #error "Unknown hardware architecture."
#endif

    // ── Helper functions ─────────────────────────────────────────────────
    // Family checks (compile-time)

    constexpr bool is_unix_family(os_id o) {
        return o == os_id::free_bsd  || o == os_id::aix        ||
               o == os_id::hpux      || o == os_id::tru64      ||
               o == os_id::nacl      || o == os_id::linux_os   ||
               o == os_id::mac_os_x  || o == os_id::net_bsd    ||
               o == os_id::open_bsd  || o == os_id::irix       ||
               o == os_id::solaris   || o == os_id::qnx_os     ||
               o == os_id::cygwin    || o == os_id::nacl       ||
               o == os_id::android   || o == os_id::gnu_hurd   ||
               o == os_id::unknown_unix;
    }

    constexpr bool is_bsd_family(os_id o) {
        return o == os_id::free_bsd || o == os_id::net_bsd ||
               o == os_id::open_bsd || o == os_id::mac_os_x;
    }

    constexpr bool is_windows_family(os_id o) {
        return o == os_id::windows_nt;
    }

    // ── Platform properties (compile-time) ──────────────────────────────

    constexpr auto native_newline() {
        if constexpr (os == os_id::windows_nt)
            return "\r\n";
        else
            return "\n";
    }

    constexpr auto path_separator() {
        if constexpr (os == os_id::windows_nt)
            return "\\";
        else
            return "/";
    }

    constexpr char path_separator_char() {
        if constexpr (os == os_id::windows_nt)
            return ';';
        else
            return ':';
    }

    constexpr auto shared_lib_prefix() {
        if constexpr (os == os_id::windows_nt)
            return "";
        else
            return "lib";
    }

    constexpr auto shared_lib_suffix() {
        if constexpr (os == os_id::windows_nt)
            return ".dll";
        else if constexpr (os == os_id::mac_os_x)
            return ".dylib";
        else if constexpr (os == os_id::hpux)
            return ".sl";
        else
            return ".so";
    }

    constexpr auto executable_suffix() {
        if constexpr (os == os_id::windows_nt)
            return ".exe";
        else
            return "";
    }

} // namespace silicon::platform

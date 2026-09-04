module;

#include <string>

#include <silicon/common.h>

module silicon.platform;




#if defined(_MSC_VER)
import silicon.platform;
#endif

namespace silicon::platform {




constexpr bool is_unix_family(os_id o) {
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

constexpr bool is_bsd_family(os_id o) {
    return o == os_id::kFreeBsd || o == os_id::kNetBsd ||
           o == os_id::kOpenBsd || o == os_id::kMacOsX;
}

constexpr bool is_windows_family(os_id o) {
    return o == os_id::kWindowsNt;
}

constexpr auto native_newline() {
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

constexpr char path_separator_char() {
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


CORE_API platform_proxy create_platform() {
    if constexpr(os == os_id::kWindowsNt) {
        return make_platform<windows_platform>();
    } else if constexpr(os == os_id::kLinuxOs) {
        return make_platform<linux_platform>();
    } else {
        return make_platform<unix_platform>();
    }
}

}

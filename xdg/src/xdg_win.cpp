// 实现单元（Windows 分支）：silicon.xdg
// 与 xdg_posix.cpp 均被 xmake 收集编译，平台选择由本文件内的 #if 守卫完成：
// 非 Windows 平台下本文件内容为空（仅模块声明），实体仅存在于对应当前平台的
// 那个文件，避免同一模块内符号重复定义。环境变量读取使用 _dupenv_s
// （MSVC 安全 CRT），避免 getenv 弃用告警。
// 内部实现函数（impl_* 前缀）与导出 API（home_dir/*Home/*Dirs）同处
// silicon::xdg 命名空间但名称不同，已消除 detail 命名空间层。
module;

#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

module silicon.xdg;

#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

namespace silicon::xdg {

std::string impl_env_or(const char *name, const std::string &def) {
    char *buf = nullptr;
    size_t len = 0;
    if(_dupenv_s(&buf, &len, name) == 0 && buf) {
        std::string s(buf, len);
        free(buf);
        if(!s.empty())
            return s;
    }
    return def;
}

std::string impl_home() {
    return impl_env_or("USERPROFILE", ".");
}

std::string impl_join(const std::string &base, const std::string &name) {
    if(base.empty())
        return name;
    if(base.back() == '\\')
        return base + name;
    return base + "\\" + name;
}

std::string impl_local_app_data() {
    return impl_env_or("LOCALAPPDATA", impl_join(impl_home(), "AppData\\Local"));
}

std::string impl_config_home() { return impl_local_app_data(); }

std::string impl_data_home() { return impl_local_app_data(); }

std::string impl_cache_home() { return impl_join(impl_local_app_data(), "cache"); }

std::string impl_state_home() { return impl_join(impl_local_app_data(), "state"); }

std::string impl_runtime_dir() { return ""; }

std::vector<std::string> impl_config_dirs() { return {}; }

std::vector<std::string> impl_data_dirs() { return {}; }

} // namespace silicon::xdg

export namespace silicon::xdg {

std::string home_dir() { return impl_home(); }

std::string config_home() { return impl_config_home(); }

std::string data_home() { return impl_data_home(); }

std::string cache_home() { return impl_cache_home(); }

std::string state_home() { return impl_state_home(); }

std::string runtime_dir() { return impl_runtime_dir(); }

std::vector<std::string> config_dirs() { return impl_config_dirs(); }

std::vector<std::string> data_dirs() { return impl_data_dirs(); }

} // namespace silicon::xdg

#endif // defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)

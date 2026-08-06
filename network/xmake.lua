add_requires("c-ares")

target("network", function()
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_os("windows") then
        -- NOMINMAX/WIN32_LEAN_AND_MEAN must be set before any Windows header
        -- (winsock2.h via the network sources) is included, otherwise
        -- minwindef.h defines max/min macros that break std::max/std::min in
        -- the coroutine headers (scheduler.hpp) this module pulls in.
        add_defines("WIN", "NOMINMAX", "WIN32_LEAN_AND_MEAN")
        add_syslinks("ws2_32", "crypt32")
    end

    if is_kind("shared") then
        add_defines("NET_SHARED_LIB", "NET_EXPORT", {public = true})
    end

    -- network 作为传统（非模块）库编译：其实现单元对头文件声明的类做外部
    -- 成员函数定义，这在 C++20 命名模块下会触发 "declaration ... follows
    -- declaration in the global module"（实现单元无法在模块作用域重新声明全局
    -- 模块里声明的类成员）。改为普通 TU 后，#include 头文件 + 外部定义即标准
    -- C++，无模块冲突。net.cppm 接口与其 :config 模块分区不再参与构建。
    -- 实现单元通过 #include 拿到 scheduler/task 等 coroutine 类型，并需在链接
    -- 时依赖 coroutine（tcp::client 调用 scheduler::poll 等）。
    add_deps("silicon::coroutine", "silicon::scheduler")

    add_includedirs("include", {public = true})
    -- Include coroutine & task headers for types used in net public headers
    add_includedirs("../coroutine/include", {public = true})
    -- Impl units #include "silicon/network_impl_includes.hpp" (the global-module
    -- fragment bundle under tools/build/shims). xmake's module dep scanner invokes
    -- the unwrapped clang-scan-deps, which does not get the include path the
    -- clang++ wrapper injects, so add it here explicitly for scanning + compile.
    -- Path is relative to THIS xmake.lua dir (pkg/silicon/network).
    add_includedirs("../../../tools/build/shims")
    add_headerfiles("include/silicon/network/**.hpp")

    add_packages("c-ares", {public = true})

    -- Include all source files, but exclude platform files for the wrong platform
    add_files("src/**.cpp")
    if is_os("windows") then
        remove_files("src/io_status_linux.cpp")
    else
        remove_files("src/io_status_win.cpp")
    end

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("network.config.cppm.in")
end)

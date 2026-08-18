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

    -- silicon::network 现为本真 C++20 命名模块库：
    --   * 接口分区 (.cppm)：network.cppm(主接口) + facade/dns/tcp/udp/tls；
    --     :config 由 network.config.cppm.in 经 xmake 生成至 $(builddir)。
    --   * 实现单元 (src/**.cpp)：均改写为 `module silicon.network;`，通过隐式
    --     导入主接口获得模块作用域内的类声明，再提供非模板成员的外联定义。
    --   * 原 25 个 .hpp 头与孤儿 net.cppm、network_impl_includes.hpp 已删除；
    --     跨模块符号（coroutine/scheduler/task）改用 `import` 而非文本包含。
    -- 依赖方向保持单向：network -> coroutine -> scheduler -> task。
    -- 错误码体系由各模块自维护：silicon.network 内置 network_error + network_category。
    add_deps("silicon::coroutine", "silicon::scheduler", "silicon::proxy")

    add_includedirs("include", {public = true})
    add_packages("c-ares", {public = true})

    -- 接口分区（主接口 + 各子分区）+ 生成的 :config 分区。
    add_files("include/silicon/network/**.cppm", {public = true})

    -- 实现单元（传统 .cpp，但均为模块实现单元）。
    -- 平台差异（io_status_linux.cpp / io_status_win.cpp）由文件内互斥 #if 守卫
    -- 选择（项目约定：不在 xmake.lua 里做 is_plat 条件 add_files/remove_files）。
    add_files("src/**.cpp")

    -- 生成的 :config 分区（版本信息），由 network.config.cppm.in 产出。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("network.config.cppm.in")
    add_files("$(builddir)/silicon/config/network.*.cppm", {public = true})
end)

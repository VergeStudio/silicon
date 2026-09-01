-- spdlog：logger 依赖，public 传递（消费方用到 logger 暴露的 spdlog 类型时仍能
-- 解析头文件）。add_requires 必须在 root scope。
add_requires("spdlog", {configs = {shared = true}})
-- c-ares：network dns/resolver 依赖，其头文件经 dns 分区的全局模块片段 <ares.h>
-- 引入，故同样 public 传递。
add_requires("c-ares")

target("silicon_core", function()
    set_kind("shared")
    set_basename("core")

    if is_plat("windows") then
        add_defines("WIN")
        -- scheduler 的 io_notifier_iocp.cpp 引用 WSAPoll；network 的 socket /
        -- tls 实现引用 WinSock2 与 CryptoAPI（证书/SSL），分别须链接 ws2_32 / crypt32。
        add_syslinks("ws2_32", "crypt32")
        -- NOMINMAX / WIN32_LEAN_AND_MEAN 必须在任何 Windows 头（network 源经由
        -- winsock2.h）之前定义，否则 minwindef.h 的 max/min 宏会破坏 std::max/min。
        add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")
    end

    -- 单 DLL 伞宏：core 编译进 core.dll，SILICON_EXPORT 由本 target 定义，
    -- 使各模块 *API（CORE_API/CONFIG_API/DI_API/EVENT_API/TIME_API/LOGGER_API/
    -- FS_API/XDG_API/JSON_API/NET_API/HTTP_API/PLUGIN_API 等）据此 dllexport
    --（消费方不定义则 dllimport）。
    add_defines("SILICON_EXPORT")

    -- 基础层（core）不依赖任何其他 silicon 模块；其他模块统一
    -- add_deps("silicon_core")，经本 target 的 public 模块 IFC 拿到
    -- 所有 silicon.X 的接口。

    add_packages("spdlog", {public = true})
    add_packages("c-ares", {public = true})

    -- 接口单元统一位于 core/include/silicon/<mod>/，实现单元统一位于
    -- core/src/<mod>/，单一 include 根即可解析全部 #include <silicon/...>。
    -- 必须公开为 {public = true}：消费方（ai.impl 等）为本 target 的 public cppm
    -- 重建 BMI 时，clang-scan-deps 扫描其 GMF 需要该路径才能解析 #include
    --（私有则 fatal error: file not found）。
    add_includedirs("include", {public = true})

    -- 全部实现单元（core 自有 + 各并入模块）。测试源随实现同置 src/<mod>/test/，
    -- 属于各自的 <mod>.test 二进制（core/src/<mod>/xmake.lua），必须排除：
    -- test_main.cpp 定义 main；ffi 为 vendored libffi，未接入构建。
    add_files("src/**.cpp")
    remove_files("src/coroutine/test/**.cpp",
                 "src/fs/test/**.cpp",
                 "src/json/test/**.cpp",
                 "src/platform/test/**.cpp",
                 "src/time/test/**.cpp",
                 "src/xdg/test/**.cpp",
                 "src/plugin/test/**.cpp",
                 "src/cli/test/**.cpp",
                 "src/ffi/**.cpp")
    -- 全部接口单元（core 自有 + 各并入模块，含 silicon.json_impl）
    add_files("include/silicon/**.cppm", {public = true})

    -- clang 对 proxy 广泛使用的 [[no_unique_address]] 误报 unknown-attribute；
    -- 消费方实例化 proxy 模板同样命中，故 public 向下传递。
    if is_config("toolchain", "clang") or is_config("toolchain", "clang-cl") then
        add_cxflags("-Wno-unknown-attributes", {public = true})
    end

    -- 生成式 :config 分区（版本信息）。core 内所有模块只保留 silicon.core:config
    -- 这一个分区，版本信息统一由 silicon.core::GetVersion* 提供。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("core.config.cppm.in")
    add_files("$(builddir)/silicon/config/core.config.cppm", {public = true})

    -- cli 保留自身 :config 分区（silicon.cli:config，命名空间 silicon::cli 的版本信息）。
    add_configfiles("cli.config.cppm.in")
    add_files("$(builddir)/silicon/config/cli.config.cppm", {public = true})
end)

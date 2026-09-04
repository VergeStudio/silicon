-- spdlog：logger 依赖，public 传递（消费方用到 logger 暴露的 spdlog 类型时仍能
-- 解析头文件）。add_requires 必须在 root scope。
add_requires("spdlog", {configs = {shared = true}})
-- c-ares：network dns/resolver 依赖，其头文件经 dns 分区的全局模块片段 <ares.h>
-- 引入，故同样 public 传递。
add_requires("c-ares")

target("core", function()
    set_kind("shared")
    set_basename("silicon_core")

    if is_plat("windows") then
        add_defines("WIN")
        -- scheduler 的 io_notifier_iocp.cpp 引用 WSAPoll；network 的 socket /
        -- tls 实现引用 WinSock2 与 CryptoAPI（证书/SSL），分别须链接 ws2_32 / crypt32。
        add_syslinks("ws2_32", "crypt32")
        -- NOMINMAX / WIN32_LEAN_AND_MEAN 必须在任何 Windows 头（network 源经由
        -- winsock2.h）之前定义，否则 minwindef.h 的 max/min 宏会破坏 std::max/min。
        add_defines("NOMINMAX", "WIN32_LEAN_AND_MEAN")
    end

    -- completion I/O 后端：--io_ring=y 时编入 io_ring_uring.cpp /
    -- io_ring_ioring.cpp；两文件内部再按平台宏互斥守卫，因此 macOS 等无后端的
    -- 平台仍只得到空 TU，io_ring 保持"声明存在但不可实例化"。
    if has_config("io_ring") then
        add_defines("SILICON_FEATURE_IO_RING=1")
        if is_plat("linux") then
            add_packages("liburing")
        end
    end

    -- 单 DLL 伞宏：core 编译进 silicon_core，CORE_EXPORT 由本 target 定义，
    -- 所有以 CORE_API 标注的导出实体据此 dllexport（消费方不定义则 dllimport）。
    add_defines("CORE_EXPORT")

    -- 基础层（core）不依赖任何其他 silicon 模块；其他模块统一
    -- add_deps("core")，经本 target 的 public 模块 IFC 拿到
    -- 所有 silicon.X 的接口。

    add_packages("spdlog", {public = true})
    add_packages("c-ares", {public = true})

    -- 接口单元统一位于 core/include/silicon/<mod>/，实现单元统一位于
    -- core/src/<mod>/，单一 include 根即可解析全部 #include <silicon/...>。
    -- 必须公开为 {public = true}：消费方（ai.impl 等）为本 target 的 public cppm
    -- 重建 BMI 时，clang-scan-deps 扫描其 GMF 需要该路径才能解析 #include
    --（私有则 fatal error: file not found）。
    add_includedirs("include", {public = true})

    -- 全部实现单元（core 自有 + 各并入模块）。各模块的测试二进制位于
    -- core/test/<mod>/，不在本 glob 覆盖范围内；需排除的是 vendored libffi
    -- 及其自带测试（ffi 未接入构建，其 test_main.cpp 另定义 main）。
    add_files("src/**.cpp")
    remove_files("src/*/test/**.cpp", "src/ffi/**.cpp")
    -- 全部接口单元（core 自有 + 各并入模块）
    add_files("include/silicon/**.cppm", {public = true})

    -- clang 对 proxy 广泛使用的 [[no_unique_address]] 误报 unknown-attribute；
    -- 消费方实例化 proxy 模板同样命中，故 public 向下传递。
    if is_config("toolchain", "clang") or is_config("toolchain", "clang-cl") then
        add_cxflags("-Wno-unknown-attributes", {public = true})
    end

    -- 生成式 :config 分区（版本信息）。core 内所有模块只保留 silicon.core:config
    -- 这一个分区，版本信息统一由 silicon.core::get_version* 提供。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("core.config.cppm.in")
    add_files("$(builddir)/silicon/config/core.config.cppm", {public = true})
end)

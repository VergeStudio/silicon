-- spdlog：原 logger 模块依赖，随 logger 并入 core。public 向下传递使消费方
-- 在用到 logger 暴露的 spdlog 类型时仍能解析头文件。add_requires 必须在 root scope。
add_requires("spdlog", {configs = {shared = true}})
-- c-ares：原 network 模块依赖（dns/resolver 异步解析），随 network 并入 core。
-- 其头文件经 dns 分区的全局模块片段 <ares.h> 引入，故同样 public 传递。
add_requires("c-ares")

-- 关于「并入 core 的模块为何不再各有独立 xmake.lua」：
-- 根 xmake.lua 以 namespace("silicon") + includes("./**") 递归拾取所有子目录的
-- xmake.lua。模块并入 core 后，其接口/实现单元统一由本文件的 glob 编译，故原
-- core/<mod>/xmake.lua 只剩两类内容：
--   1) 声明 <mod>.test 二进制 target（依赖 silicon::core + silicon::test）——
--      仅 coroutine / fs / json / platform / plugin / time / xdg 七个模块有
--      test/ 或 specs/ 目录，这些文件保留；
--   2) 纯注释占位（config / di / event / logger / scheduler / scheduler-task）——
--      这些模块无 test/specs，文件内不声明任何 target，对构建零影响
--      （实证：删除前后 xmake show -l targets 均为同一份 17 目标清单）。
-- 为免"看似有配置实则无作用"造成误导，第 2 类已删除，其说明并入本注释块：
--   * config / di / event / logger / scheduler / scheduler.task：接口单元与实现
--     单元均已由本 target 的 glob 编译；不再各自生成 :config 分区（版本信息统一
--     由 silicon.core::GetVersion* 提供）。消费方直接 add_deps("core") 即可
--     import silicon.config / silicon.di / silicon.event / silicon.logger /
--     silicon.scheduler / silicon.scheduler.task，无需再单独依赖任何子模块 target。
--   * logger 额外说明：其 spdlog 依赖已由本文件 add_packages("spdlog", {public=true})
--     向下传递，消费方使用 logger 暴露的 spdlog 类型时仍可解析头文件。

target("core", function()
    set_kind("shared")
    set_basename("core")

    -- silicon.thread 已并入 silicon.scheduler；scheduler（含 task）与 coroutine
    -- 亦已并入本 target（见下），thread_pool / task<T> / mutex / event 等统一经
    -- silicon.scheduler / silicon.scheduler.task / silicon.coroutine 模块消费。

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
    --（消费方不定义则 dllimport）。旧 per-module 双宏 CORE_SHARED_LIB/CORE_EXPORT 已弃用。
    add_defines("SILICON_EXPORT")

    -- 基础层（core）不依赖任何其他 silicon 模块：platform / util / exception /
    -- library / proxy / error / config / di / event / time / logger / fs / xdg /
    -- json / scheduler（含 task）/ coroutine / network / http / plugin 现已统一
    -- 在 core 内编译，对外保持原 module 名不变（silicon.platform / silicon.config /
    -- ... / silicon.network / silicon.http / silicon.plugin），消除以往
    -- core → X → core 的跨 target 循环依赖。其他模块统一 add_deps("core") 即可
    -- 消费上述基础模块，且经本 target 的 public 模块 IFC 拿到所有 silicon.X 的接口。

    add_packages("spdlog", {public = true})
    add_packages("c-ares", {public = true})

    -- 目录约定（2026-08-31 归一）：所有并入模块的接口单元统一位于
    -- core/include/silicon/<mod>/，实现单元统一位于 core/src/<mod>/。
    -- 单一 include 根即可解析全部 #include <silicon/...>；必须 {public = true}：
    -- 超级项目侧 clang 消费方（ai.impl/cli.impl/app）会为本 target 的 public cppm
    -- 重建 BMI，clang-scan-deps 扫描其 GMF 时需要该路径才能解析 #include
    --（私有则 fatal error: file not found）。
    add_includedirs("include", {public = true})

    -- 全部实现单元（core 自有 + 各并入模块）
    add_files("src/**.cpp")
    -- 全部接口单元（core 自有 + 各并入模块，含 silicon.json_impl）
    add_files("include/silicon/**.cppm", {public = true})

    -- 随模块一同分发的公共头文件（*API 伞宏与 proxy dispatch 宏等）
    add_headerfiles("include/silicon/core/**.h")
    add_headerfiles("include/silicon/coroutine/**.h")
    add_headerfiles("include/silicon/logger/**.h")
    add_headerfiles("include/silicon/network/**.h")
    add_headerfiles("include/silicon/proxy/**.h")

    -- clang 对 MSFT proxy 广泛使用的 [[no_unique_address]] 误报 unknown-attribute，
    -- 沿用原 proxy target 的处理（消费方实例化 proxy 模板同样命中，故 public 向下传递）。
    if is_config("toolchain", "clang") or is_config("toolchain", "clang-cl") then
        add_cxflags("-Wno-unknown-attributes", {public = true})
    end

    -- 生成式 :config 分区（版本信息）。core 内所有模块（含并入的
    -- config/event/di/logger/scheduler/task/coroutine/network）只保留
    -- silicon.core:config 这一个分区——版本信息统一由 silicon.core::GetVersion*
    -- 提供，其余模块一律使用它，不再各自生成 *.config.cppm。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("core.config.cppm.in")
    add_files("$(builddir)/silicon/config/core.config.cppm", {public = true})
end)

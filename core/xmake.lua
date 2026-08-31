-- spdlog：原 logger 模块依赖，随 logger 并入 core。public 向下传递使消费方
-- 在用到 logger 暴露的 spdlog 类型时仍能解析头文件。add_requires 必须在 root scope。
add_requires("spdlog", {configs = {shared = true}})

target("core", function()
    set_kind("shared")
    set_basename("core")

    -- silicon.thread 已并入 silicon.scheduler；scheduler（含 task）与 coroutine
    -- 亦已并入本 target（见下），thread_pool / task<T> / mutex / event 等统一经
    -- silicon.scheduler / silicon.scheduler.task / silicon.coroutine 模块消费。

    if is_plat("windows") then
        add_defines("WIN")
        -- scheduler 的 io_notifier_iocp.cpp（随 scheduler 并入）引用 WSAPoll，
        -- 须链接 ws2_32。
        add_syslinks("ws2_32")
    end

    -- 单 DLL 伞宏：core 编译进 core.dll，SILICON_EXPORT 由本 target 定义，
    -- 使各模块 *API（CORE_API/CONFIG_API/DI_API/EVENT_API/TIME_API/LOGGER_API/
    -- FS_API/XDG_API/JSON_API 等）据此 dllexport（消费方不定义则 dllimport）。
    -- 旧 per-module 双宏 CORE_SHARED_LIB/CORE_EXPORT 已弃用。
    add_defines("SILICON_EXPORT")

    -- 基础层（core）不依赖任何其他 silicon 模块：platform / util / exception /
    -- library / proxy / error / config / di / event / time / logger / fs / xdg /
    -- json / scheduler（含 task）/ coroutine 现已统一在 core 内编译，对外保持原
    -- module 名不变（silicon.platform / silicon.config / ... / silicon.json /
    -- silicon.scheduler / silicon.scheduler.task / silicon.coroutine），消除以往
    -- core → X → core 的跨 target 循环依赖。其他模块统一 add_deps("core") 即可
    -- 消费上述基础模块，且经本 target 的 public 模块 IFC 拿到所有 silicon.X 的接口。

    add_packages("spdlog", {public = true})

    add_includedirs("include", {public = true})
    -- 各并入模块（已物理移入 core/ 子目录）的 include 根：其 .cpp/.cppm 内的
    -- #include <silicon/X/...> 解析。必须 {public = true}：超级项目侧 clang 消费方
    -- （ai.impl/cli.impl/app）会为本 target 的 public cppm 重建 BMI，clang-scan-deps
    -- 扫描其 GMF 时需要这些路径才能解析 #include（私有则 fatal error: file not found）。
    add_includedirs("config/include", {public = true})
    add_includedirs("di/include", {public = true})
    add_includedirs("event/include", {public = true})
    add_includedirs("time/include", {public = true})
    add_includedirs("logger/include", {public = true})
    add_includedirs("fs/include", {public = true})
    add_includedirs("xdg/include", {public = true})
    add_includedirs("json/include", {public = true})
    add_includedirs("scheduler/include", {public = true})
    add_includedirs("scheduler/task/include", {public = true})
    add_includedirs("coroutine/include", {public = true})

    -- core 自有源文件
    add_files("src/**.cpp")
    -- 平台差异（shared_library_unix.cpp / shared_library_windows.cpp）由文件内
    -- 互斥 #if 守卫选择（项目约定：不在 xmake.lua 里做 is_plat 条件 add_files）。
    -- 核心模块接口：含 platform / exception / util（均在 core/include/silicon/core
    -- 下，不再被 remove_files 排除）。
    add_files("include/silicon/core/**.cppm", {public = true})

    -- 并入的 9 个基础模块（已物理移入 core/ 子目录）：保持 silicon.X 模块名不变，
    -- 仅把文件加入本 target 编译，从而零 import 改动、零分区重命名。各模块 .cppm
    -- 为 public（消费方据此 import）。
    -- config
    add_files("config/include/silicon/config/**.cppm", {public = true})
    add_files("config/src/**.cpp")
    -- di（纯模板，无 .cpp）
    add_files("di/include/silicon/di/**.cppm", {public = true})
    -- event
    add_files("event/include/silicon/event/**.cppm", {public = true})
    add_files("event/src/**.cpp")
    -- time
    add_files("time/include/silicon/time/**.cppm", {public = true})
    add_files("time/src/**.cpp")
    -- logger
    add_files("logger/include/silicon/logger/**.cppm", {public = true})
    add_files("logger/src/**.cpp")
    add_headerfiles("logger/include/silicon/logger/**.h")
    -- fs
    add_files("fs/include/silicon/fs/**.cppm", {public = true})
    add_files("fs/src/**.cpp")
    -- xdg（两平台实现单元均收集编译，平台选择由各文件内 #if 守卫完成）
    add_files("xdg/include/silicon/xdg/**.cppm", {public = true})
    add_files("xdg/src/**.cpp")
    -- json（纯模块库：json 模块 + json_impl 模块，含全量内联实现）
    add_files("json/include/silicon/json/**.cppm", {public = true})
    add_files("json/include/silicon/json_impl/json_impl.cppm", {public = true})
    -- scheduler（含 task，已物理移入 core/scheduler/）：moduleonly+static 目标
    -- silicon::scheduler / silicon::task 撤销，实体随 core.dll 导出。三平台
    -- io_notifier 实现单元均收集编译，平台选择由各文件内 SILICON_PLATFORM_*
    -- 守卫完成；Windows 下 ws2_32 见上方 add_syslinks。
    add_files("scheduler/include/silicon/scheduler/**.cppm", {public = true})
    add_files("scheduler/src/**.cpp")
    add_files("scheduler/task/include/silicon/scheduler/task/**.cppm", {public = true})
    add_files("scheduler/task/src/**.cpp")
    -- coroutine（已物理移入 core/coroutine/）：moduleonly+static 目标撤销，
    -- 接口（silicon.coroutine 主接口 + 16 分区 + silicon.coroutine.error）与实现
    -- 单元随 core.dll 编译导出。依赖 scheduler/task/proxy/error 均已在 core 内。
    add_files("coroutine/include/silicon/coroutine/**.cppm", {public = true})
    add_files("coroutine/src/**.cpp")
    add_headerfiles("coroutine/include/silicon/coroutine/**.h")

    -- clang 对 MSFT proxy 广泛使用的 [[no_unique_address]] 误报 unknown-attribute，
    -- 沿用原 proxy target 的处理（消费方实例化 proxy 模板同样命中，故 public 向下传递）。
    if is_config("toolchain", "clang") or is_config("toolchain", "clang-cl") then
        add_cxflags("-Wno-unknown-attributes", {public = true})
    end

    -- 生成式 :config 分区（版本信息）。core 内所有模块（含并入的
    -- config/event/di/logger/scheduler/task/coroutine）只保留 silicon.core:config
    -- 这一个分区——版本信息统一由 silicon.core::GetVersion* 提供，其余模块一律
    -- 使用它，不再各自生成 *.config.cppm。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("core.config.cppm.in")
    add_files("$(builddir)/silicon/config/core.config.cppm", {public = true})
end)

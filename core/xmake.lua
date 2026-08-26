target("core", function()
    set_kind("$(kind)")

    -- silicon.thread 已并入 silicon.scheduler，而 silicon.scheduler 依赖
    -- silicon.coroutine（后者又依赖 core），故 core 不再重导出/依赖该模块，
    -- 避免形成循环依赖。需要 thread_pool 的消费方直接 import silicon.scheduler。

    if is_plat("windows") then
        add_defines("WIN")
    end

    -- 单 DLL 伞宏：core 恒编译进聚合 silicon.dll，SILICON_EXPORT 由本 target 定义，
    -- 使 CORE_API（common.h 已重指向 SILICON_EXPORT）据此 dllexport
    -- （core.config.cppm 的 GetVersion* 等据此导出，否则退化为 dllimport 触发 C2491）。
    -- 旧 per-module 双宏 CORE_SHARED_LIB/CORE_EXPORT 已弃用。
    add_defines("SILICON_EXPORT")

    -- 基础层（core）不依赖任何其他 silicon 模块：platform / exception / error /
    -- proxy / util 现已统一在 core 内编译，对外保持原 module 名不变
    -- （silicon.platform / silicon.exception / silicon.error / silicon.proxy /
    -- silicon.util），消除以往 core → platform → proxy → core 的跨 target 循环依赖。
    -- 其他模块统一 add_deps("core") 即可消费上述基础模块。

    add_includedirs("include", {public = true})

    add_files("src/**.cpp")
    -- 平台差异（shared_library_unix.cpp / shared_library_windows.cpp）由文件内
    -- 互斥 #if 守卫选择（项目约定：不在 xmake.lua 里做 is_plat 条件 add_files）。
    -- 核心模块接口：含 platform / exception / util（均在 core/include/silicon/core
    -- 下，不再被 remove_files 排除）。
    add_files("include/silicon/core/**.cppm", {public = true})

    -- clang 对 MSFT proxy 广泛使用的 [[no_unique_address]] 误报 unknown-attribute，
    -- 沿用原 proxy target 的处理（消费方实例化 proxy 模板同样命中，故 public 向下传递）。
    if is_config("toolchain", "clang") or is_config("toolchain", "clang-cl") then
        add_cxflags("-Wno-unknown-attributes", {public = true})
    end

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("core.config.cppm.in")
    add_files("$(builddir)/silicon/config/core.*.cppm", {public = true})


end)

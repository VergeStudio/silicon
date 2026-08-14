target("core", function()
    set_kind("$(kind)")

    -- silicon.thread 已并入 silicon.scheduler，而 silicon.scheduler 依赖
    -- silicon.coroutine（后者又依赖 core），故 core 不再重导出/依赖该模块，
    -- 避免形成循环依赖。需要 thread_pool 的消费方直接 import silicon.scheduler。

    if is_plat("windows") then
        add_defines("WIN")
    end

    if is_kind("shared") then
        add_defines("CORE_SHARED_LIB", "CORE_EXPORT", {public = true})
    end

    -- 动态库语义错误码现由 silicon.library 模块自身提供（library_error / make_error_code）。

    -- silicon.platform / silicon.exception 已从 core 目标抽离为独立 target：
    -- proxy 只依赖 silicon.exception（而非整个 core），借此打破
    -- core → platform → proxy → core 的循环依赖（与 util 抽离同机制）。
    add_deps("silicon::platform")
    add_deps("silicon::exception")

    add_includedirs("include", {public = true})

    add_files("src/**.cpp")
    -- 平台差异（shared_library_unix.cpp / shared_library_windows.cpp）由文件内
    -- 互斥 #if 守卫选择（项目约定：不在 xmake.lua 里做 is_plat 条件 add_files）。
    add_files("include/silicon/core/**.cppm", {public = true})
    -- platform.cppm / exception.cppm 已由独立 target 编译（避免与各自
    -- standalone target 重复定义同一模块），core 经 add_deps 复用其 BMI。
    remove_files("include/silicon/core/platform/platform.cppm")
    remove_files("include/silicon/core/exception/exception.cppm")

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("core.config.cppm.in")
    add_files("$(builddir)/silicon/config/core.*.cppm", {public = true})


end)

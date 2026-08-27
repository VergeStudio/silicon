target("scheduler", function()
    set_kind("moduleonly")

    if is_os("windows") then
        add_defines("WIN")
    end

    -- task 目录已并入 scheduler/task（target 名仍为 task）。
    -- 注意：不得再依赖 silicon::coroutine —— coroutine 现在反向依赖本 target。
    -- scheduler_facade 走 silicon.proxy 类型擦除（proxy 已并入 core），依赖 core 即可。
    -- 错误码体系由各模块自维护：silicon.scheduler 内置 scheduler_error。
    add_deps("silicon::task", "core")

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 SCHEDULER_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_includedirs("include")

    add_files("include/silicon/scheduler/**.cppm", {public = true})

    -- 生成式 :config 分区（版本信息），由 scheduler.config.cppm.in 产出。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("scheduler.config.cppm.in")
    add_files("$(builddir)/silicon/config/scheduler.*.cppm", {public = true})
end)

target("scheduler.impl", function()
    set_kind("static")

    add_deps("silicon::scheduler")

    add_defines("SILICON_EXPORT")

    -- Windows IOCP 通知器（io_notifier_iocp.cpp）引用 WSAPoll，须链接 ws2_32。
    if is_plat("windows") then
        add_syslinks("ws2_32")
    end

    add_files("src/**.cpp")
end)

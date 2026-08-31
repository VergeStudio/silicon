target("coroutine", function()
    set_kind("moduleonly")

    if is_os("windows") then
        add_defines("WIN")
    end

    -- 依赖方向单向：coroutine -> scheduler（scheduler/task/scheduler.impl/
    -- task.impl 目标已撤销，scheduler 与 task 实体随 core.dll 导出）。
    -- 错误码体系由各模块自维护：silicon.coroutine 内置 coroutine_error / channel_error。
    add_deps("core")

    -- coroutine 为独立 moduleonly+static 目标，不编译进 DLL；本目标须定义 SILICON_EXPORT，
    -- 否则 :config 生成分区里的 COROUTINE_API 版本函数退化为 dllimport 触发 C2491。
    add_defines("SILICON_EXPORT")

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/coroutine/**.hpp")

    add_files("include/silicon/coroutine/**.cppm", {public = true})

    -- 生成式 :config 分区（版本信息），由 coroutine.config.cppm.in 产出。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("coroutine.config.cppm.in")
    add_files("$(builddir)/silicon/config/coroutine.*.cppm", {public = true})
end)

target("coroutine.impl", function()
    set_kind("static")

    add_deps("silicon::coroutine")

    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)

target("coroutine.test", function()
    set_kind("binary")
    -- coroutine 接口单向依赖 scheduler/task，且 coroutine 实体（sync_wait_event /
    -- pipe_t）实际编译于 scheduler 模块——现随 core.dll 导出，链接 core 导入库即可。
    add_deps("silicon::coroutine", "silicon::coroutine.impl",
             "core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

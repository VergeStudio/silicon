target("task", function()
    set_kind("moduleonly")

    if is_os("windows") then
        add_defines("WIN")
    end

    add_deps("core")

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 TASK_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_includedirs("include", {public = true})

    add_files("include/silicon/scheduler/task/**.cppm", {public = true})

    -- 生成式 :config 分区（版本信息），由 task.config.cppm.in 产出。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("task.config.cppm.in")
    add_files("$(builddir)/silicon/config/task.*.cppm", {public = true})
end)

target("task.impl", function()
    set_kind("static")

    add_deps("silicon::task")

    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)

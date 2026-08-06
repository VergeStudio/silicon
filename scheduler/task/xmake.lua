target("task", function()
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_os("windows") then
        add_defines("WIN")
    end

    if is_kind("shared") then
        add_defines("TASK_SHARED_LIB", "TASK_EXPORT", {public = true})
    end

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/scheduler/task/**.hpp")

    add_files("src/**.cpp")
    add_files("include/silicon/scheduler/task/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("task.config.cppm.in")
    add_files("$(builddir)/silicon/config/task.*.cppm", {public = true})


end)

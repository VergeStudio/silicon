target("thread", function()
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_os("windows") then
        add_defines("WIN")
    end

    -- NOTE: set_kind("$(kind)") means the target-scope is_kind() cannot match
    -- "shared"; must query the global config variable via is_config() instead.
    if is_config("kind", "shared") then
        add_defines("THREAD_SHARED_LIB", "THREAD_EXPORT", {public = true})
    end

    -- IPool / pool 已折叠进 thread.cppm 模块接口（单一接口，无分区，避免
    -- clang 22 加载 ≥10 个 BMI 时断言崩溃）；原 include/silicon/thread/*.hpp
    -- 已删除，仅保留 common.h 供全局模块片段使用。
    add_includedirs("include")

    add_deps("silicon::task", {configs = {shared = true}})

    add_files("src/**.cpp")
    add_files("include/silicon/thread/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("thread.config.cppm.in")
    add_files("$(builddir)/silicon/config/thread.*.cppm", {public = true})

end)

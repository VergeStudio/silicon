add_requires("spdlog", {configs = {shared = true}})

target("logger", function()
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_os("windows") then
        add_defines("WIN")
    end

    if is_config("kind", "shared") then
        add_defines("LOGGER_SHARED_LIB", "LOGGER_EXPORT", {public = true})
    end

    add_packages("spdlog", {public = true})

    add_deps("silicon::core", {configs = {shared = true}})

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/logger/**.h")

    add_files("src/**.cpp")
    add_files("include/silicon/logger/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("logger.config.cppm.in")
    add_files("$(builddir)/silicon/config/logger.*.cppm", {public = true})
end)

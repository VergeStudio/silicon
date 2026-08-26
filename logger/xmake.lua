add_requires("spdlog", {configs = {shared = true}})

target("logger", function()
    set_kind("moduleonly")

    if is_os("windows") then
        add_defines("WIN")
    end

    add_packages("spdlog", {public = true})

    add_deps("silicon::core")

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 LOGGER_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/logger/**.h")

    add_files("include/silicon/logger/**.cppm", {public = true})

    -- 生成式 :config 分区（版本信息），由 logger.config.cppm.in 产出。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("logger.config.cppm.in")
    add_files("$(builddir)/silicon/config/logger.*.cppm", {public = true})
end)

target("logger.impl", function()
    set_kind("static")

    add_deps("silicon::logger")

    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)

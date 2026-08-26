target("config", function()
    set_kind("moduleonly")

    -- include/silicon/config/json/loader.cppm 依赖 silicon.json 模块 BMI（全限定名，跨命名空间可解析）。
    -- 错误码体系由 silicon.config.error 子模块自维护（config::result 别名亦定义于此）。
    add_deps("silicon::json", "silicon::fs", "core")

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 CONFIG_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_includedirs("include")

    add_files("include/silicon/config/**.cppm", {public = true})

    -- 生成式 :config 分区（版本信息），由 config.config.cppm.in 产出。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("config.config.cppm.in")
    add_files("$(builddir)/silicon/config/config.*.cppm", {public = true})
end)

target("config.impl", function()
    set_kind("static")

    -- 单向依赖接口模块（moduleonly）；绝不反向。
    add_deps("silicon::config")

    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)

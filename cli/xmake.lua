target("cli", function()
    set_kind("moduleonly")

    add_deps("core")

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 CLI_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_includedirs("include", {public = true})

    add_files("include/silicon/cli/**.cppm", {public = true})

    -- 生成式 :config 分区（版本信息），由 cli.config.cppm.in 产出。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("cli.config.cppm.in")
    add_files("$(builddir)/silicon/config/cli.*.cppm", {public = true})
end)

target("cli.impl", function()
    set_kind("static")

    add_deps("silicon::cli")

    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)

target("cli.test", function()
    set_kind("binary")
    add_deps("silicon::cli", "silicon", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

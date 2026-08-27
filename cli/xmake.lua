target("cli", function()
    set_kind("moduleonly")

    add_deps("core")

    -- 单 DLL 伞宏闸门：本模块为独立 moduleonly+static，定义 SILICON_EXPORT 使
    -- CLI_API 实体 dllexport，由消费方静态链接。
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
    add_deps("silicon::cli", "silicon::cli.impl", "core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

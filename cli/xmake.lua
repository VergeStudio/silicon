target("cli", function()
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end
    -- cli_error 枚举与 make_error_code 内置在 silicon.cli.parser.interface 子模块中。
    add_includedirs("include", {public = true})
    add_files("src/**.cpp")
    add_files("include/silicon/cli/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("cli.config.cppm.in")
    add_files("$(builddir)/silicon/config/cli.*.cppm", {public = true})
end)

target("cli.test", function()
    set_kind("binary")
    add_deps("silicon::cli", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

target("ai", function()
    -- 模块库：符号经由 .cppm 模块接口导出，shared 构建无需传统导出宏。
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end
    add_deps("silicon::json", "silicon::http", "silicon::di", "silicon::core", "silicon::proxy")
    add_files("include/silicon/ai/**.cppm", {public = true})
    add_files("src/**.cpp")

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("ai.config.cppm.in")
    add_files("$(builddir)/silicon/config/ai.*.cppm", {public = true})
end)

target("ai.test", function()
    set_kind("binary")
    add_deps("silicon::ai", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

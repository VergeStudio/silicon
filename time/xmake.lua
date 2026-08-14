target("time", function()
    -- 模块库：符号经由 .cppm 模块接口导出，shared 构建无需传统导出宏。
    set_kind("$(kind)")

    -- clock / date_source 门面走 silicon.proxy 类型擦除，需依赖 proxy 模块。
    add_deps("silicon::proxy")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end
    add_files("src/**.cpp")
    add_files("include/silicon/time/**.cppm", {public = true})
end)

target("time.test", function()
    set_kind("binary")
    add_deps("silicon::time", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

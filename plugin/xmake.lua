target("plugin", function()
    -- 模块库：符号经由 .cppm 模块接口导出，shared 构建无需传统导出宏。
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end
    -- silicon.proxy：插件是跨 DLL/ABI 边界，采用类型擦除替代虚表继承。
    -- proxy 的 dispatch 宏走头文件通道，故同时需要其 public includedirs。
    add_deps("silicon::proxy")
    add_deps("silicon::error")

    add_files("include/silicon/plugin/**.cppm", {public = true})
    add_files("src/**.cpp")
end)

target("plugin.test", function()
    set_kind("binary")
    add_deps("silicon::plugin", "silicon::test", "silicon::error")
    add_files("test/**.cpp")
    add_tests()
end)

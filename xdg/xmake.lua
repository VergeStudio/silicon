target("xdg", function()
    -- 模块库：符号经由 .cppm 模块接口导出，shared 构建无需传统导出宏。
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end
    add_deps("core")
    add_includedirs("include", {public = true})
    add_files("include/silicon/xdg/**.cppm", {public = true})
    -- 两平台实现单元均收集编译；平台选择由各文件内 #if 守卫完成
    -- （xdg_win.cpp: _WIN32 系；xdg_posix.cpp: 其余），非目标平台时内容为空。
    add_files("src/**.cpp")
end)

target("xdg.test", function()
    set_kind("binary")
    add_deps("silicon::xdg", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

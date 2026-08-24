target("tui", function()
    -- 模块库：符号经由 .cppm 模块接口导出，shared 构建无需传统导出宏。
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end
    add_deps("silicon::core")
    -- TUI 门面基于 silicon.proxy 的 type-erasure（取消 i_terminal/i_pty/i_tui_renderer 抽象基类）。
    add_deps("silicon::proxy")
    add_deps("silicon::error")
    add_files("include/silicon/tui/**.cppm", {public = true})
    add_files("src/**.cpp")
end)

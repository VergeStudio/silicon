target("tui", function()
    -- 接口模块（moduleonly）：纯抽象接口（proxy 门面），不含实现。
    set_kind("moduleonly")

    -- TUI 门面基于 silicon.proxy 的 type-erasure（proxy 已并入 core，无需再单独依赖）。
    add_deps("silicon::core")

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 TUI_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_files("include/silicon/tui/**.cppm", {public = true})
end)

target("tui.impl", function()
    set_kind("static")

    add_deps("silicon::tui")

    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)

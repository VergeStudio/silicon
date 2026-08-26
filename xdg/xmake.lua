target("xdg", function()
    -- 接口模块（moduleonly）：纯抽象接口，不含实现。
    set_kind("moduleonly")

    add_deps("core")

    add_includedirs("include", {public = true})

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 XDG_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_files("include/silicon/xdg/**.cppm", {public = true})
end)

target("xdg.impl", function()
    set_kind("static")

    add_deps("silicon::xdg")

    add_defines("SILICON_EXPORT")

    -- 两平台实现单元均收集编译；平台选择由各文件内 #if 守卫完成。
    add_files("src/**.cpp")
end)

target("xdg.test", function()
    set_kind("binary")
    add_deps("silicon::xdg", "silicon", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

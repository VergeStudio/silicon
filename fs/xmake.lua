target("fs", function()
    -- 接口模块（moduleonly）：纯抽象接口，不含实现。
    set_kind("moduleonly")

    -- 错误码体系（result = std::expected<T, std::error_code>）。
    add_deps("silicon::core")

    add_includedirs("include", {public = true})

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 FS_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_files("include/silicon/fs/**.cppm", {public = true})
end)

target("fs.impl", function()
    set_kind("static")

    add_deps("silicon::fs")

    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)

target("fs.test", function()
    set_kind("binary")
    add_deps("silicon::fs", "silicon", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

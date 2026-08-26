target("plugin", function()
    -- 接口模块（moduleonly）：纯抽象接口（proxy 门面 + 工厂签名），不含实现。
    set_kind("moduleonly")

    -- silicon.proxy：插件是跨 DLL/ABI 边界，采用类型擦除替代虚表继承。
    -- proxy 的 dispatch 宏走头文件通道，故同时需要其 public includedirs（经 core 传递）。
    add_deps("core")

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 PLUGIN_API（SILICON_EXPORT）dllexport。
    add_defines("SILICON_EXPORT")

    add_files("include/silicon/plugin/**.cppm", {public = true})
end)

target("plugin.impl", function()
    set_kind("static")

    add_deps("silicon::plugin")

    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)

target("plugin.test", function()
    set_kind("binary")
    add_deps("silicon::plugin", "silicon", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

target("ai", function()
    -- 接口模块（moduleonly）：纯抽象接口（proxy 门面 + 工厂签名 + 类声明），不含实现。
    set_kind("moduleonly")

    -- 接口（silicon.http 等）与实现符号由 silicon_core 统一提供。
    add_deps("core")

    -- 单 DLL 伞宏闸门：本模块实体经 CORE_API（CORE_EXPORT）dllexport，
    -- 由消费方（测试/应用）静态链接 silicon_core 与 ai.impl.lib。
    add_defines("CORE_EXPORT")

    add_includedirs("include", {public = true})

    add_files("include/silicon/ai/**.cppm", {public = true})

    -- 生成式 :config 分区（版本信息），由 ai.config.cppm.in 产出。
    set_configdir("$(builddir)/silicon/config")
    add_configfiles("ai.config.cppm.in")
    add_files("$(builddir)/silicon/config/ai.*.cppm", {public = true})
end)

target("ai.impl", function()
    set_kind("static")

    add_deps("silicon::ai")

    add_defines("CORE_EXPORT")

    add_files("src/**.cpp")
end)

target("ai.test", function()
    set_kind("binary")
    -- ai.impl 为独立 static lib，测试须直接链接以解析 ai 实体符号。
    add_deps("silicon::ai", "silicon::ai.impl", "core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

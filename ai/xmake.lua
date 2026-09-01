target("ai", function()
    -- 接口模块（moduleonly）：纯抽象接口（proxy 门面 + 工厂签名 + 类声明），不含实现。
    set_kind("moduleonly")

    -- silicon.http 已并入 core：其接口（silicon.http / silicon.http.error）与
    -- 实现现由 core.dll 统一提供，经 core 依赖的 public IFC 拿到，无需再单独依赖。
    add_deps("silicon_core")

    -- 单 DLL 伞宏闸门：本模块实体经 *_API（SILICON_EXPORT）dllexport，
    -- 由消费方（测试/应用）静态链接 core.dll 与 ai.impl.lib。
    add_defines("SILICON_EXPORT")

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

    add_defines("SILICON_EXPORT")

    add_files("src/**.cpp")
end)

target("ai.test", function()
    set_kind("binary")
    -- silicon.json 已并入 core，其模块 IFC 经 core 的 public 依赖传递（无需直接 add_deps）。
    -- ai.impl 为独立 static lib（不再并入任何 DLL），测试须直接链接以解析 ai 实体符号。
    add_deps("silicon::ai", "silicon::ai.impl", "silicon_core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

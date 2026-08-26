target("ai", function()
    -- 接口模块（moduleonly）：纯抽象接口（proxy 门面 + 工厂签名 + 类声明），不含实现。
    set_kind("moduleonly")

    add_deps("silicon::json", "silicon::http", "silicon::di", "core")

    -- 单 DLL 伞宏：本模块编译进 silicon.dll 时，实体经 AI_API（SILICON_EXPORT）dllexport。
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
    -- json is moduleonly: 其 BMI 经直接依赖传递（MSVC C7612 规避）。
    add_deps("silicon::ai", "silicon::json", "silicon", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

-- platform 模块已并入 core（silicon.platform 在 core 内统一编译，接口 →
-- core/include/silicon/platform，实现 → core/src/platform），此处仅保留其
-- 单元测试 target。core 现编译进 core.dll，故依赖 core。
target("platform.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

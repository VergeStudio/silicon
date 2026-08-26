-- platform 模块已并入 core（silicon.platform 在 core 内统一编译），
-- 此处仅保留其单元测试 target。core 现编译进单一聚合 silicon.dll，故依赖 silicon。
target("platform.test", function()
    set_kind("binary")
    add_deps("silicon", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

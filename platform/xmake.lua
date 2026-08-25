-- platform 模块已并入 core（silicon.platform 在 core 内统一编译），
-- 此处仅保留其单元测试 target。
target("platform.test", function()
    set_kind("binary")
    add_deps("core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

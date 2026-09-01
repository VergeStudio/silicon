-- time.test：time 模块的测试二进制（模块接口与符号由 silicon_core 提供）。
target("time.test", function()
    set_kind("binary")
    -- system_clock.h（header-only，global module 实体）经 core 的 public include 根解析。
    add_deps("silicon::silicon_core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

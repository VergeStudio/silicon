-- time.test：time 模块的测试二进制（模块接口与符号由 core 提供）。
target("time.test", function()
    set_kind("binary")
    -- system_clock 经 silicon.time.system_clock 子模块（silicon.time facade re-export）提供。
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

-- config.test：config 模块的测试二进制（config_value 与错误 category 由 core 提供）。
target("config.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

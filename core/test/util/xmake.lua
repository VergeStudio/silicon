-- util.test：util 模块的测试二进制（纯函数工具由 core 提供）。
target("util.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

-- event.test：event 模块的测试二进制（event 类与错误 category 由 core 提供）。
target("event.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

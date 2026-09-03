-- scheduler.test：scheduler 模块的测试二进制（调度器与原语由 core 提供）。
target("scheduler.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

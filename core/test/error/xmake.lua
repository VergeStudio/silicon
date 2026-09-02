-- error.test：error 模块的测试二进制（统一错误返回类型契约由 core 提供）。
target("error.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

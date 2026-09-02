-- di.test：di 模块的测试二进制（依赖注入容器由 core 提供）。
target("di.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

-- proxy.test：proxy 模块的测试二进制（类型擦除基础设施全部随 core 导出）。
target("proxy.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

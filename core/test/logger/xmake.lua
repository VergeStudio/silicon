-- logger.test：logger 模块的测试二进制（init / 日志函数 / 错误 category 由 core 提供）。
target("logger.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

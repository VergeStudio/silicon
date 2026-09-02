-- library.test：library 模块的测试二进制（shared_library 与错误 category 由 core 提供）。
target("library.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

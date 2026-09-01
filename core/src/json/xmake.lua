-- json.test：json 模块的测试二进制（模块接口与符号由 core 提供）。
target("json.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

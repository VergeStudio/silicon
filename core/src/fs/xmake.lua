-- fs.test：fs 模块的测试二进制（模块接口与符号由 silicon_core 提供）。
target("fs.test", function()
    set_kind("binary")
    add_deps("silicon::silicon_core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

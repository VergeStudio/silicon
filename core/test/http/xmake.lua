-- http.test：http 模块的测试二进制（header-only 值类型由本 TU #include，
-- 模块接口与符号由 core 提供）。
target("http.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("*.cpp")
    add_tests()
end)

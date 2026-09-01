-- coroutine.test：coroutine 模块的测试二进制。
-- coroutine 已并入 core（接口/实现由 core/xmake.lua 的 glob 编译，
-- 见 core/include/silicon/coroutine/ 与 core/src/coroutine/），测试随实现
-- 同置本目录（test/），依赖 silicon::silicon_core 获取接口 IFC 与 core.dll 符号。
target("coroutine.test", function()
    set_kind("binary")
    add_deps("silicon::silicon_core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

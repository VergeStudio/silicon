-- json 模块已并入 core（由 core 单一 DLL 编译导出）。其接口单元（silicon.json /
-- silicon.json_impl）现统一在 core/xmake.lua 内处理。本文件仅保留测试目标，
-- 重指向 silicon::silicon_core（silicon.json 的 IFC 与符号现均由 core.dll 提供）。
target("json.test", function()
    set_kind("binary")
    add_deps("silicon::silicon_core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

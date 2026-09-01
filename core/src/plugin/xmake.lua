-- plugin 模块已并入 core（由 core 单一 DLL 编译导出）。其接口单元
-- （silicon.plugin / silicon.plugin.error）与实现单元现已统一在
-- core/xmake.lua 内处理（接口 → core/include/silicon/plugin，实现 → core/src/plugin）。
-- 本文件仅保留测试目标，重指向 silicon::silicon_core（plugin 接口与符号均由 core.dll 提供）。
target("plugin.test", function()
    set_kind("binary")
    add_deps("silicon::silicon_core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

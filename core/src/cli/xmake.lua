-- cli 模块已并入 core（由 core 单一 DLL 编译导出）。其接口单元
-- （silicon.cli 及 :config 分区）与实现单元现已统一在 core/xmake.lua 内处理
-- （接口 → core/include/silicon/cli，实现 → core/src/cli，config 模板 → core/cli.config.cppm.in）。
-- 本文件仅保留测试目标，重指向 silicon::silicon_core（cli 接口与符号均由 core.dll 提供）。
target("cli.test", function()
    set_kind("binary")
    add_deps("silicon::silicon_core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

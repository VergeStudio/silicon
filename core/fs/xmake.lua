-- fs 模块已并入 core（由 core 单一 DLL 编译导出）。其接口单元（silicon.fs /
-- silicon.fs.error）与实现单元（src/**.cpp）现统一在 core/xmake.lua 内处理。
-- 本文件仅保留测试目标，重指向 silicon::core。
target("fs.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

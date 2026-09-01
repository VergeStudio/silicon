-- time 模块已并入 core（由 core 单一 DLL 编译导出）。其接口单元（silicon.time）
-- 与实现单元（src/time.cpp）现统一在 core/xmake.lua 内处理。本文件仅保留测试目标，
-- 重指向 silicon::core（silicon.time 的 IFC 与符号现均由 core.dll 提供）。
target("time.test", function()
    set_kind("binary")
    -- 消费方模式（xmake C++ modules 推荐）：silicon.time 接口与符号现均来自 core.dll，
    -- 故只依赖 silicon::core 与测试支撑 silicon::test 即可。
    -- system_clock.h（header-only，global module 实体）经 core 的 public
    -- include 根（core/include，见 core/xmake.lua）解析，无需在此重复声明。
    add_deps("silicon::core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

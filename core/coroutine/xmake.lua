-- coroutine 模块已并入 core（由 core 单一 DLL 编译导出）。其接口单元
-- （silicon.coroutine / silicon.coroutine:*/ silicon.coroutine.error）与实现单元
-- （src/**.cpp）现统一在 core/xmake.lua 内处理；:config 版本分区已收敛
-- （版本信息由 silicon.core::GetVersion* 统一提供，本模块不再生成）。
-- 本文件仅保留测试目标，重指向 silicon::core（coroutine 接口与符号均由
-- core.dll 提供，且 coroutine 单向依赖 scheduler/task，实体现随 core.dll 导出）。
target("coroutine.test", function()
    set_kind("binary")
    add_deps("silicon::core", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

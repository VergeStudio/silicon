set_project("silicon")

-- set xmake min version
set_xmakever("3.0.0")

-- version
set_version("0.0.1", {build = "%Y%m%d%H%M"})

-- set warning all as error
--set_warnings("all", "error")

set_languages("c17", "cxx23") -- https://xmake.io/#/zh-cn/manual/project_target?id=targetset_languages

-- 设置默认编码
set_encodings("utf-8")

add_rules("mode.debug", "mode.release", "mode.valgrind")

-- 工具链选择
if is_plat("windows") then
    set_toolchains("msvc")

    if is_mode("release") then
        --set_optimize("smallest")
        add_ldflags("/LTCG")
    end

    add_defines("SILICON_PLATFORM_WINDOWS=1")
else
    set_toolchains("clang")

    add_defines("SILICON_PLATFORM_UNIX=1")
    if is_plat("linux") then
        add_defines("SILICON_PLATFORM_LINUX=1")
    elseif is_plat("macosx") then
        add_defines("SILICON_PLATFORM_APPLE=1")
    elseif is_plat("bsd") then
        add_defines("SILICON_PLATFORM_BSD=1")
    end
end

namespace("silicon", function()
    -- 单一聚合 DLL：链接所有子模块 static 实现目标，对外导出 moduleonly 接口 IFC。
    -- 必须在 includes("./**") 之前声明，使子模块 test 目标可前向依赖本聚合 DLL
    -- （xmake 在 namespace 内解析 add_deps 前向引用，但依赖目标须先于使用方可见）。
    -- 试点阶段仅接入 silicon.time；#189 接入全部子模块 static 实现目标。
    target("silicon", function()
        set_kind("shared")
        set_basename("silicon")

        -- 单 DLL 伞宏：编译进本 DLL 的所有目标据此 dllexport（非 public，不向消费方传播）。
        -- 各模块 *API（TIME_API/CORE_API）已重指向 SILICON_EXPORT，实体据此显式 dllexport。
        -- 注意：聚合 DLL 无自有编译单元，utils.symbols.export_all 扫描不到符号会生成空 .def
        -- 触发 LNK1104，故不挂该 rule，完全依赖显式 *API 标注。
        add_defines("SILICON_EXPORT")

        -- 聚合 DLL 入口桩：提供真实目标文件，使 MSVC 链接 CRT 启动符 _DllMainCRTStartup
        --（无自有目标文件时仅 .lib 输入会触发 LNK4001/LNK2001）。
        add_files("silicon_dll.cpp")

        -- 聚合所有子模块 static 实现目标（public：其模块 IFC 经此向消费方传递）。
        -- core（proxy 门面来源，混合 .cppm+.cpp 模块目标）【绝不】直接 add_deps 本 shared
        -- 目标——否则触发 xmake module-batch 崩溃（attempt to index a nil value
        -- 'cxx_sourcebatch'）；core 经各 X.impl→X→core 传递链作为 PRIVATE 链接依赖进入 DLL。
        -- di 为纯模板 static（无 .cpp），其 .lib 亦经此链接进 DLL；json 为纯 moduleonly
        -- 无 .lib，消费方仅 import 其 IFC，不在此列表。
        add_deps("silicon::time.impl", {public = true})
        add_deps("silicon::event.impl", {public = true})
        add_deps("silicon::config.impl", {public = true})
        add_deps("silicon::ai.impl", {public = true})
        add_deps("silicon::cli.impl", {public = true})
        add_deps("silicon::coroutine.impl", {public = true})
        add_deps("silicon::network.impl", {public = true})
        add_deps("silicon::scheduler.impl", {public = true})
        add_deps("silicon::task.impl", {public = true})
        add_deps("silicon::plugin.impl", {public = true})
        add_deps("silicon::tui.impl", {public = true})
        add_deps("silicon::logger.impl", {public = true})
        add_deps("silicon::http.impl", {public = true})
        add_deps("silicon::fs.impl", {public = true})
        add_deps("silicon::xdg.impl", {public = true})
        add_deps("silicon::di", {public = true})
    end)

    includes("./**")
end)

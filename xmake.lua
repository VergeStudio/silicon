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
    -- 基础模块（config/di/event/time/logger/fs/xdg/json/scheduler+task/coroutine）
    -- 已并入 core，由 core 这一单一 DLL 导出；其余模块（network/plugin/tui/
    -- ai/cli/http 等）仍为独立 moduleonly + static 目标，消费方 add_deps("core")
    -- 链接 core.dll。子模块 test 目标经由 includes("./**") 解析 core 依赖。
    includes("./**")
end)

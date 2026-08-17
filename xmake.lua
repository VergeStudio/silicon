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

if is_mode("release") then
    --set_optimize("smallest")
    if is_plat("windows") then
        add_ldflags("/LTCG")
    end
end

-- 工具链选择：
--   Windows 使用 MSVC —— MSVC 原生随附 C++ 标准库模块（import std; 可直接使用），
--   而本机 clang 无预编译 std 模块 BMI，无法编译 import std;。
--   非 Windows 平台继续用 clang。
if is_plat("windows") then
    set_toolchains("msvc")
else
    set_toolchains("clang")
end

-- ── 平台宏（全仓唯一来源 / single source of truth）──────────────────────────
-- SILICON_PLATFORM_* 在构建系统顶层统一定义，供所有模块（含 C++20 模块实现单元、
-- 全局模块片段 GMF 区、10 个平台分离文件，以及原先因宏未定义而失效的
-- fs.cpp / tui.cpp / tui.cppm 分支）直接使用，无需 #include。
-- 仅覆盖 OS 族；编译器宏（_MSC_VER / __GNUC__ / __clang__ 等）不在此列。
-- 注意：silicon.platform 模块另以 constexpr 枚举做编译期 OS 检测，二者正交。
if is_plat("windows") then
    add_defines("SILICON_PLATFORM_WINDOWS=1")
else
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
    includes("./**")
end)

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

-- completion I/O 后端（Linux=io_uring / Windows=I/O Ring）。
-- 默认关闭：关闭时 :io_ring 接口单元仍参与编译（便于跨平台语法校验），但两个
-- 后端实现单元为空 TU，io_ring 无可用定义，消费方一律走 io_notifier
--（IOCP / epoll / kqueue）。
option("io_ring")
    set_default(false)
    set_showmenu(true)
    set_description("Enable the platform completion I/O ring backend (Windows: I/O Ring, Linux: io_uring)")
option_end()

-- add_requires 必须在 root scope；仅开启选项且目标平台为 Linux 时拉取 liburing，
-- 避免其他平台在 config 阶段去解析一个用不到的包。
if has_config("io_ring") and is_plat("linux") then
    add_requires("liburing")
end

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
    -- 各模块统一并入 core（接口 core/include/silicon/<mod>/，实现
    -- core/src/<mod>/，测试 core/test/<mod>/，vendored libffi 同置
    -- core/src/ffi/）；仅 ai（moduleonly + static）独立。各 <mod>.test target
    -- 声明在 core/test/<mod>/xmake.lua。
    includes("./**")
end)

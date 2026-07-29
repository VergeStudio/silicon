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

set_toolchains("clang")

namespace("silicon", function()
    includes("./**")
end)

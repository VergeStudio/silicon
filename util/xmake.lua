-- silicon.util: standalone module target (extracted from core so that
-- silicon.exception and other modules can depend on it without creating an
-- intra-target module-name resolution problem inside the silicon.core target).
target("util", function()
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_kind("shared") then
        -- CORE_API resolves to visibility("default") when CORE_SHARED_LIB is
        -- defined; empty CORE_EXPORT avoids any `extern 1` style expansion.
        add_defines("CORE_SHARED_LIB", "CORE_EXPORT=", {public = true})
    end

    add_includedirs("../core/include", {public = true})

    add_files("../core/src/util/util.cppm", {public = true})
    add_files("../core/src/util/util.cpp")
end)

-- silicon.platform: standalone module target (extracted from core so that
-- silicon.library's impl units (shared_library*.cpp) can `import silicon.platform`
-- without the intra-target module-resolution problem inside the silicon.core target).
-- Source stays in ../core/src/platform (consistent with how silicon.util /
-- silicon.exception were extracted).
target("platform", function()
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_kind("shared") then
        -- Mirrors silicon.util / silicon.exception: CORE_API resolves to
        -- visibility("default") when CORE_SHARED_LIB is defined; empty CORE_EXPORT
        -- avoids any `extern 1` style expansion.
        add_defines("CORE_SHARED_LIB", "CORE_EXPORT=", {public = true})
    end

    add_includedirs("../core/include", {public = true})

    -- platform 门面基于 silicon.proxy 的 type-erasure（取消 i_platform 抽象基类）。
    add_deps("silicon::proxy")

    add_files("../core/include/silicon/core/platform/facade.cppm", {public = true})
end)

target("platform.test", function()
    set_kind("binary")
    add_deps("silicon::platform", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

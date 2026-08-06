target("json", function()
    -- 模块库：符号经由 .cppm 模块接口导出，shared 构建无需传统导出宏。
    set_kind("$(kind)")

    -- Windows DLL：C++20 模块附着实体不隐式 inline，MSVC 目标无自动导出，
    -- 统一用 .def 全量导出，保证消费方可跨 DLL 链接模块符号。
    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/json/**.hpp")
    add_headerfiles("include/silicon/json_impl/**.hpp")

    -- silicon.exception 模块由 core 目标编译（core/include/silicon/core/exception/exception.cppm），
    -- 独立的 silicon::exception target 并不存在，依赖 core 以获取其 BMI。
    add_deps("silicon::core")

    add_files("include/silicon/json/**.cppm", {public = true})
end)

target("json.test", function()
    set_kind("binary")
    add_deps("silicon::json", "silicon::test")
    add_files("test/**.cpp")
    add_tests()
end)

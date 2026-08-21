target("config", function()
    set_kind("$(kind)")

    -- include/silicon/config/json/loader.cppm 依赖 silicon.json 模块 BMI（全限定名，跨命名空间可解析）。
    -- 错误码体系由 silicon.config.error 子模块自维护（config::result 别名亦定义于此）。
    add_deps("silicon::json", "silicon::fs", {configs = {shared = true}})
    add_deps("silicon::error")

    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_config("kind", "shared") then
        add_defines("CONFIG_SHARED_LIB", "CONFIG_EXPORT", {public = true})
    end

    add_includedirs("include")

    add_files("src/**.cpp")
    add_files("include/silicon/config/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("config.config.cppm.in")
    add_files("$(builddir)/silicon/config/config.*.cppm", {public = true})
end)

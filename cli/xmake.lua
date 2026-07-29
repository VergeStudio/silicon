target("cli", function()
    set_kind("$(kind)")

    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_config("kind", "shared") then
        add_defines("CLI_SHARED_LIB", "CLI_EXPORT", {public = true})
    end

    add_includedirs("include")

    add_files("src/**.cpp")
    add_files("src/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("cli.config.cppm.in")
    add_files("$(builddir)/silicon/config/cli.*.cppm", {public = true})
end)

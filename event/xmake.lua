target("event", function()
    set_kind("$(kind)")
    add_deps("core")

    if is_plat("windows") and is_config("kind", "shared") then
        add_rules("utils.symbols.export_all", {export_classes = true})
    end

    if is_config("kind", "shared") then
        add_defines("EVENT_SHARED_LIB", "EVENT_EXPORT", {public = true})
    end

    add_includedirs("include")

    add_files("src/**.cpp")
    add_files("include/silicon/event/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("event.config.cppm.in")
    add_files("$(builddir)/silicon/config/event.*.cppm", {public = true})
end)

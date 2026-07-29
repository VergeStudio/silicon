target("di", function()
    set_kind("$(kind)")

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/di/**.h")

    add_files("src/**.cpp")
    add_files("src/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("di.config.cppm.in")
    add_files("$(builddir)/silicon/config/di.*.cppm", {public = true})
end)

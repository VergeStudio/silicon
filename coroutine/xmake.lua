target("coroutine", function()
    set_kind("$(kind)")

    if is_plat("windows") then
        add_defines("WIN")
    end

    if is_kind("shared") then
        add_defines("COROUTINE_SHARED_LIB", "COROUTINE_EXPORT", {public = true})
    end

    add_deps("core", "task", "thread", "network", {configs = {shared = true}})

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/coroutine/**.hpp")

    add_files("src/**.cpp")
    add_files("src/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("coroutine.config.cppm.in")
    add_files("$(builddir)/silicon/config/coroutine.*.cppm", {public = true})


end)

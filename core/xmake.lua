target("core", function()
    set_kind("$(kind)")

    -- 全限定名：跨命名空间入口（app 侧 target）解析传递依赖时，裸名 "thread"
    -- 会以入口 target 的命名空间上下文查找而失败（dep target not found）。
    add_deps("silicon::thread")

    if is_plat("windows") then
        add_defines("WIN")
    end

    if is_kind("shared") then
        add_defines("CORE_SHARED_LIB", "CORE_EXPORT", {public = true})
    end

    add_includedirs("include", {public = true})
    add_headerfiles("include/silicon/core/**.hpp")

    add_files("src/**.cpp")
    -- Platform-specific shared library implementation
    if is_plat("windows") then
        add_files("src/shared_library/shared_library_windows.cpp")
        remove_files("src/shared_library/shared_library_unix.cpp", "src/shared_library/shared_library_hpux.cpp", "src/shared_library/shared_library_vx.cpp")
    else
        add_files("src/shared_library/shared_library_unix.cpp")
        remove_files("src/shared_library/shared_library_windows.cpp", "src/shared_library/shared_library_hpux.cpp", "src/shared_library/shared_library_vx.cpp")
    end
    add_files("include/silicon/core/**.cppm", {public = true})

    set_configdir("$(builddir)/silicon/config")
    add_configfiles("core.config.cppm.in")
    add_files("$(builddir)/silicon/config/core.*.cppm", {public = true})


end)
